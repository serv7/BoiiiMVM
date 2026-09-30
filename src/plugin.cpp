#include "backend.hpp"
#include "binds.hpp"
#include "capture.hpp"
#include "reshade_capture.hpp"
#include "overlay_hook.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <kiero/kiero.h>
#include <MH/MinHook.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace {
using present_fn=HRESULT(__stdcall*)(IDXGISwapChain*,UINT,UINT);
using resize_fn=HRESULT(__stdcall*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
present_fn original_present=nullptr;
resize_fn original_resize=nullptr;
ID3D11Device* device=nullptr;
ID3D11DeviceContext* context=nullptr;
ID3D11RenderTargetView* target=nullptr;
IDXGISwapChain* active_swap=nullptr;
HWND window=nullptr;
WNDPROC old_wndproc=nullptr;
ImGuiContext* imgui_context=nullptr;
std::atomic<bool> stopping=false;
std::thread init_thread;
bool swallowed[256]{};
using set_cursor_fn=BOOL(WINAPI*)(int,int);
using clip_cursor_fn=BOOL(WINAPI*)(const RECT*);
set_cursor_fn original_set_cursor=nullptr;
clip_cursor_fn original_clip_cursor=nullptr;
bool cursor_owned=false;
std::atomic<bool> cursor_hooks=false;
std::recursive_mutex cursor_mutex;
RECT previous_clip{};
bool mouse_free() {return window && (theater::menu_open.load() || theater::exit_dialog.load()) && GetForegroundWindow()==window;}
BOOL WINAPI set_cursor(int x,int y) {
    if(mouse_free())return TRUE;
    return original_set_cursor(x,y);
}
BOOL WINAPI clip_cursor(const RECT* rect) {
    return original_clip_cursor(mouse_free()?nullptr:rect);
}
void sync_mouse_capture() {
    std::lock_guard lock(cursor_mutex);
    if(theater::menu_open || theater::exit_dialog || (window && GetForegroundWindow()!=window))theater::clear_camera_lift();
    if(!cursor_hooks)return;
    if(mouse_free() && !cursor_owned) {
        GetClipCursor(&previous_clip);cursor_owned=true;
        original_clip_cursor(nullptr);
        if(GetCapture()==window)ReleaseCapture();
        theater::log_message("Tab cursor released");
    } else if(!mouse_free() && cursor_owned) {
        original_clip_cursor(GetForegroundWindow()==window?&previous_clip:nullptr);
        cursor_owned=false;theater::log_message("Tab cursor returned to game");
    }
}
bool initialize_cursor_hooks() {
    auto a=MH_CreateHook(reinterpret_cast<void*>(SetCursorPos),set_cursor,reinterpret_cast<void**>(&original_set_cursor));
    if(a!=MH_OK)return false;
    auto b=MH_CreateHook(reinterpret_cast<void*>(ClipCursor),clip_cursor,reinterpret_cast<void**>(&original_clip_cursor));
    if(b!=MH_OK) {MH_RemoveHook(reinterpret_cast<void*>(SetCursorPos));return false;}
    if(MH_EnableHook(reinterpret_cast<void*>(SetCursorPos))!=MH_OK || MH_EnableHook(reinterpret_cast<void*>(ClipCursor))!=MH_OK) {
        MH_DisableHook(reinterpret_cast<void*>(SetCursorPos));MH_DisableHook(reinterpret_cast<void*>(ClipCursor));
        MH_RemoveHook(reinterpret_cast<void*>(SetCursorPos));MH_RemoveHook(reinterpret_cast<void*>(ClipCursor));return false;
    }
    cursor_hooks=true;theater::log_message("Tab cursor recenter / clip hooks enabled");return true;
}
bool dispatch_binding(theater::binds::id selected,WPARAM key) {
    using namespace theater;
    auto state=current_snapshot();
    if(!state.playing || GetForegroundWindow()!=window)return false;
    if(exit_dialog)return true;
    if(selected==binds::id::controls) {clear_camera_lift();hud_visible=!hud_visible.load(); if(!hud_visible)menu_open=false;return true;}
    if(selected==binds::id::menu) {clear_camera_lift();close_camera_editor();menu_open=!menu_open.load();if(menu_open)hud_visible=true;return true;}
    if(selected==binds::id::record){capture::toggle();return true;}
    if(selected==binds::id::exit_film){clear_camera_lift();exit_dialog=true;menu_open=false;hud_visible=true;return true;}
    if(menu_open) {
        if(key==VK_ESCAPE)menu_open=false;
        return true;
    }
    switch(selected) {
    case binds::id::game_hud:enqueue(action::toggle_game_hud);return true;
    case binds::id::view:clear_camera_lift();enqueue(action::cycle_view);return true;
    case binds::id::lift_down:case binds::id::lift_up:
        set_camera_lift_key(selected==binds::id::lift_up,state.view==2 && (state.free_mode==0 || state.free_mode==1));return true;
    case binds::id::place:if(state.view==2)enqueue(action::interact);return true;
    case binds::id::delete_all:enqueue(action::erase_all);return true;
    case binds::id::seek_back:enqueue(action::seek_back);return true;
    case binds::id::seek_forward:enqueue(action::seek_forward);return true;
    case binds::id::speed_up:enqueue(action::speed_up);return true;
    case binds::id::speed_down:enqueue(action::speed_down);return true;
    case binds::id::pause:enqueue(action::pause);return true;
    case binds::id::first_camera:enqueue(action::first_camera);return true;
    case binds::id::dolly:if(state.view==2){clear_camera_lift();enqueue(action::dolly_camera);}return true;
    case binds::id::edit:if(state.view==2){clear_camera_lift();enqueue(action::free_camera);}return true;
    case binds::id::roll_left:if(state.view==2)enqueue_roll(-1);return true;
    case binds::id::roll_right:if(state.view==2)enqueue_roll(1);return true;
    case binds::id::zoom_in:if(state.view==2)enqueue_zoom(1);return true;
    case binds::id::zoom_out:if(state.view==2)enqueue_zoom(-1);return true;
    default:return key=='Q' || key=='E'; // Native Q can restart the demo.
    }
}
bool capture_key(WPARAM key) {
    if(theater::binds::listening()){
        if(key==VK_ESCAPE){theater::binds::cancel();return true;}
        theater::binds::accept({int(key),theater::binds::modifiers()});return true;
    }
    return dispatch_binding(theater::binds::match({int(key),theater::binds::modifiers()}),key);
}
LRESULT CALLBACK wndproc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(imgui_context)ImGui::SetCurrentContext(imgui_context);
    if(msg==WM_KILLFOCUS) {theater::clear_camera_lift();theater::menu_open=false;theater::exit_dialog=false;std::fill(std::begin(swallowed),std::end(swallowed),false);}
    if((msg==WM_KEYUP || msg==WM_SYSKEYUP)){
        if(w==theater::binds::get(theater::binds::id::lift_down).key)theater::set_camera_lift_key(false,false);
        if(w==theater::binds::get(theater::binds::id::lift_up).key)theater::set_camera_lift_key(true,false);
    }
    if(msg==WM_LBUTTONUP || msg==WM_RBUTTONUP){
        int key=msg==WM_LBUTTONUP?theater::binds::mouse_left:theater::binds::mouse_right;
        if(key==theater::binds::get(theater::binds::id::lift_down).key)theater::set_camera_lift_key(false,false);
        if(key==theater::binds::get(theater::binds::id::lift_up).key)theater::set_camera_lift_key(true,false);
    }
    bool is_theater=theater::current_snapshot().playing;
    if(is_theater && (msg==WM_KEYDOWN || msg==WM_SYSKEYDOWN) && w<256) {
        if((theater::menu_open || theater::exit_dialog) && imgui_context && !theater::binds::listening())ImGui_ImplWin32_WndProcHandler(hwnd,msg,w,l);
        if(!(l&(1LL<<30))) {
            if(w==VK_ESCAPE && !theater::menu_open && !theater::binds::listening()){
                if(theater::exit_dialog)theater::exit_dialog=false;
                else {theater::clear_camera_lift();theater::exit_dialog=true;theater::hud_visible=true;}
                swallowed[w]=true;
            } else swallowed[w]=capture_key(w);
        }
        else if(swallowed[w]){
            auto bind=theater::binds::match({int(w),theater::binds::modifiers()});
            if(bind==theater::binds::id::seek_back || bind==theater::binds::id::seek_forward ||
                bind==theater::binds::id::speed_up || bind==theater::binds::id::speed_down)capture_key(w);
        }
        if(swallowed[w]) {sync_mouse_capture();return 0;}
    }
    if((msg==WM_KEYUP||msg==WM_SYSKEYUP) && w<256 && swallowed[w]) {
        if(theater::menu_open && imgui_context)ImGui_ImplWin32_WndProcHandler(hwnd,msg,w,l);
        swallowed[w]=false;return 0;
    }
    if(is_theater && theater::binds::listening() && (msg==WM_LBUTTONDOWN || msg==WM_RBUTTONDOWN || msg==WM_MOUSEWHEEL)){
        int key=msg==WM_LBUTTONDOWN?theater::binds::mouse_left:msg==WM_RBUTTONDOWN?theater::binds::mouse_right:
            GET_WHEEL_DELTA_WPARAM(w)>0?theater::binds::wheel_up:theater::binds::wheel_down;
        theater::binds::accept({key,theater::binds::modifiers()});return 0;
    }
    if(is_theater && !theater::menu_open && !theater::exit_dialog) {
        if(msg==WM_MOUSEWHEEL) {
            float notches=float(GET_WHEEL_DELTA_WPARAM(w))/WHEEL_DELTA;
            auto bind=theater::binds::match({notches>0?theater::binds::wheel_up:theater::binds::wheel_down,theater::binds::modifiers()});
            if(bind==theater::binds::id::zoom_in || bind==theater::binds::id::zoom_out){theater::enqueue_zoom((bind==theater::binds::id::zoom_in?1.f:-1.f)*std::abs(notches));return 0;}
            if(bind==theater::binds::id::roll_left || bind==theater::binds::id::roll_right){theater::enqueue_roll((bind==theater::binds::id::roll_right?1.f:-1.f)*std::abs(notches));return 0;}
            if(dispatch_binding(bind,0))return 0;
        }
        if(msg==WM_LBUTTONDOWN || msg==WM_RBUTTONDOWN){
            auto bind=theater::binds::match({msg==WM_LBUTTONDOWN?theater::binds::mouse_left:theater::binds::mouse_right,theater::binds::modifiers()});
            if(dispatch_binding(bind,0))return 0;
        }
        if(theater::current_snapshot().view==2 && (msg==WM_LBUTTONUP || msg==WM_RBUTTONUP))return 0;
    }
    if(is_theater && theater::menu_open && (msg==WM_LBUTTONDOWN || msg==WM_RBUTTONDOWN || msg==WM_MOUSEWHEEL)){
        int key=msg==WM_LBUTTONDOWN?theater::binds::mouse_left:msg==WM_RBUTTONDOWN?theater::binds::mouse_right:
            GET_WHEEL_DELTA_WPARAM(w)>0?theater::binds::wheel_up:theater::binds::wheel_down;
        auto bound=theater::binds::match({key,theater::binds::modifiers()});
        if(bound==theater::binds::id::menu || bound==theater::binds::id::controls || bound==theater::binds::id::record){
            if(dispatch_binding(bound,0)){sync_mouse_capture();return 0;}
        }
    }
    if(is_theater && (theater::menu_open || theater::exit_dialog) && imgui_context) {
        ImGui_ImplWin32_WndProcHandler(hwnd,msg,w,l);
        switch(msg) {
        case WM_MOUSEMOVE:case WM_MOUSEWHEEL:case WM_LBUTTONDOWN:case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:case WM_RBUTTONUP:case WM_CHAR:return 0;
        }
    }
    sync_mouse_capture();
    return CallWindowProcW(old_wndproc,hwnd,msg,w,l);
}
bool create_target(IDXGISwapChain* swap) {
    ID3D11Texture2D* buffer=nullptr;
    if(FAILED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer))))return false;
    HRESULT result=device->CreateRenderTargetView(buffer,nullptr,&target);buffer->Release();
    return SUCCEEDED(result);
}
bool initialize_renderer(IDXGISwapChain* swap) {
    DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(swap->GetDesc(&desc)) || !desc.OutputWindow)return false;
    if(FAILED(swap->GetDevice(__uuidof(ID3D11Device),reinterpret_cast<void**>(&device))))return false;
    device->GetImmediateContext(&context);window=desc.OutputWindow;active_swap=swap;
    imgui_context=ImGui::CreateContext();ImGui::SetCurrentContext(imgui_context);
    ImGui::GetIO().IniFilename=nullptr;ImGui::GetIO().LogFilename=nullptr;
    theater::configure_ui();
    if(!ImGui_ImplWin32_Init(window) || !ImGui_ImplDX11_Init(device,context))return false;
    theater::reshade_capture::initialize();
    old_wndproc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(wndproc)));
    theater::log_message("D3D11 UI initialized");return create_target(swap);
}
void render_overlay(ID3D11RenderTargetView* output) {
    if(!imgui_context || !context || stopping)return;
    ID3D11RenderTargetView* destination=output?output:target;
    if(!destination)return;
    auto previous=ImGui::GetCurrentContext();ImGui::SetCurrentContext(imgui_context);
    ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
    sync_mouse_capture();ImGui::GetIO().MouseDrawCursor=theater::menu_open.load() || theater::exit_dialog.load();
    theater::set_diagnostic_stage("draw_ui");theater::draw_ui();ImGui::Render();theater::set_diagnostic_stage("idle");sync_mouse_capture();
    ID3D11RenderTargetView* saved=nullptr;ID3D11DepthStencilView* depth=nullptr;
    context->OMGetRenderTargets(1,&saved,&depth);
    context->OMSetRenderTargets(1,&destination,nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    context->OMSetRenderTargets(1,&saved,depth);
    if(saved)saved->Release();if(depth)depth->Release();
    ImGui::SetCurrentContext(previous);
}
HRESULT __stdcall present(IDXGISwapChain* swap,UINT sync,UINT flags) {
    if(!stopping){
        if(!imgui_context)initialize_renderer(swap);
        if(imgui_context && swap==active_swap){
            if(!target)create_target(swap);
            theater::reshade_capture::begin_present();
            theater::capture::present(swap,device,context,theater::current_snapshot().playing);
            if(!theater::reshade_capture::active() || !theater::reshade_capture::frame_seen()){
                theater::reshade_capture::mark_pre_render();render_overlay(nullptr);
            }
        }
    }
    return original_present(swap,sync,flags);
}
HRESULT __stdcall resize(IDXGISwapChain* swap,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags) {
    if(swap==active_swap)theater::capture::stop();
    if(swap==active_swap && target) {target->Release();target=nullptr;}
    return original_resize(swap,count,width,height,format,flags);
}
}
namespace theater {
void render_after_reshade(ID3D11RenderTargetView* output){render_overlay(output);}
}
extern "C" __declspec(dllexport) const char* p_name() {return "BO3 Theater Keyboard UI 0.8.1";}
extern "C" __declspec(dllexport) void post_unpack() {
    if(init_thread.joinable())return;
    init_thread=std::thread([] {
        std::filesystem::create_directories("MVM");theater::binds::load();theater::capture::load_settings();theater::log_message("Plugin loaded");
        theater::initialize_diagnostics();
        // Wait until the client's other post_unpack hooks are installed.
        for(int i=0;i<20 && !stopping;++i)Sleep(100);
        if(stopping)return;
        if(!theater::initialize_backend()) {theater::log_message("Initialization rejected; game unchanged");return;}
        if(!initialize_cursor_hooks())theater::log_message("Cursor-release hooks could not be installed");
        for(int i=0;i<100 && !stopping;++i) {
            auto status=kiero::init(kiero::RenderType::D3D11);
            if(status==kiero::Status::Success) {
                if(kiero::bind(8,reinterpret_cast<void**>(&original_present),present)!=kiero::Status::Success ||
                   kiero::bind(13,reinterpret_cast<void**>(&original_resize),resize)!=kiero::Status::Success) {
                    theater::log_message("D3D11 hooks failed");theater::shutdown_backend();
                }
                return;
            }
            Sleep(100);
        }
        theater::shutdown_backend();
    });
}
extern "C" __declspec(dllexport) void pre_destroy() {
    stopping=true;if(init_thread.joinable())init_thread.join();
    theater::reshade_capture::shutdown();
    theater::capture::shutdown();
    theater::shutdown_backend();
    sync_mouse_capture();
    if(cursor_hooks) {
        MH_DisableHook(reinterpret_cast<void*>(SetCursorPos));MH_DisableHook(reinterpret_cast<void*>(ClipCursor));
        MH_RemoveHook(reinterpret_cast<void*>(SetCursorPos));MH_RemoveHook(reinterpret_cast<void*>(ClipCursor));cursor_hooks=false;
    }
    theater::shutdown_diagnostics();
    if(window && old_wndproc && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window,GWLP_WNDPROC))==wndproc)
        SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(old_wndproc));
    kiero::unbind(8);kiero::unbind(13);
    // Do not globally disable hooks: the bot menu and ReShade own other hooks.
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(instance);
    return TRUE;
}
