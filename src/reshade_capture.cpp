#include "reshade_capture.hpp"
#include "capture.hpp"
#include "backend.hpp"
#include "overlay_hook.hpp"
#include "vendor/reshade/reshade_events.hpp"
#include <Windows.h>
#include <Psapi.h>
#include <d3d11.h>
#include <algorithm>
#include <atomic>

extern "C" __declspec(dllexport) const char* NAME="BO3 Theater Capture";
extern "C" __declspec(dllexport) const char* DESCRIPTION="Captures the finished ReShade frame for BO3 theater videos.";

namespace theater::reshade_capture {
namespace {
using register_addon_fn=bool(*)(void*,uint32_t);
using register_event_fn=void(*)(void*,reshade::addon_event,void*);
using unregister_addon_fn=void(*)(void*);
HMODULE reshade_module=nullptr,own_module=nullptr;
std::atomic<bool> registered=false;
bool attempted=false;
std::atomic<bool> seen_frame=false;
std::atomic<unsigned long long> generation=0,last_finished=~0ull;
std::atomic<unsigned long long> pre_rendered=~0ull;
// The overlay is drawn after ReShade's effects when ReShade reports them, otherwise at present.
// "Reports them" is decided per frame: with effects toggled off the callback simply stops, and
// waiting on a timeout made the Tab menu and the controls vanish along with the effects.
std::atomic<bool> finished_since_present=false,finished_last_frame=false,presenting=false;

void on_finish(reshade::api::effect_runtime* runtime,reshade::api::command_list*,
    reshade::api::resource_view view,reshade::api::resource_view){
    if(!registered || !runtime || !view.handle)return;
    auto* source=runtime->get_device();
    if(!source || source->get_api()!=reshade::api::device_api::d3d11)return;
    auto resource=source->get_resource_from_view(view);
    if(!resource.handle)return;
    auto* texture=reinterpret_cast<ID3D11Texture2D*>(resource.handle);
    ID3D11Device* device=nullptr;texture->GetDevice(&device);
    if(!device)return;
    ID3D11DeviceContext* context=nullptr;device->GetImmediateContext(&context);
    if(context){
        auto frame=generation.load();
        if(last_finished.exchange(frame)==frame){context->Release();device->Release();return;}
        // Only a draw made earlier in this same present call counts; one from the previous frame doesn't.
        bool already_drawn=presenting.load() && pre_rendered.load()==frame;
        bool first=!seen_frame.exchange(true);
        if(first)log_message("ReShade finish-effects callback received; clean capture begins next frame");
        finished_since_present=true;
        capture::set_post_effect_source(true);
        if(!first && !already_drawn)capture::post_effect(texture,device,context);
        if(!already_drawn)render_after_reshade(reinterpret_cast<ID3D11RenderTargetView*>(view.handle));
        context->Release();
    }
    device->Release();
}
HMODULE find_reshade(){
    HMODULE modules[512]{};DWORD bytes=0;
    if(!K32EnumProcessModules(GetCurrentProcess(),modules,sizeof(modules),&bytes))return nullptr;
    for(size_t i=0;i<std::min<size_t>(bytes/sizeof(HMODULE),std::size(modules));++i)
        if(GetProcAddress(modules[i],"ReShadeRegisterAddon") && GetProcAddress(modules[i],"ReShadeRegisterEventForAddon"))return modules[i];
    return nullptr;
}
}
void initialize(){
    if(attempted)return;attempted=true;
    reshade_module=find_reshade();
    if(!reshade_module){log_message("ReShade add-on API not found; game-frame capture remains available");return;}
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&initialize),&own_module))return;
    auto reg=reinterpret_cast<register_addon_fn>(GetProcAddress(reshade_module,"ReShadeRegisterAddon"));
    auto event=reinterpret_cast<register_event_fn>(GetProcAddress(reshade_module,"ReShadeRegisterEventForAddon"));
    if(!reg || !event || !reg(own_module,20)){
        log_message("ReShade add-on registration refused; install the full add-on support build for effect capture");return;
    }
    event(own_module,reshade::addon_event::reshade_finish_effects,reinterpret_cast<void*>(&on_finish));
    registered=true;
    log_message("ReShade finish-effects capture registered");
}
void shutdown(){
    if(registered){
        registered=false;capture::set_post_effect_source(false);
        if(auto unregister=reinterpret_cast<unregister_addon_fn>(GetProcAddress(reshade_module,"ReShadeUnregisterAddon")))unregister(own_module);
    }
}
bool active(){return registered;}
bool frame_seen(){return seen_frame && finished_last_frame;}
void begin_present(){++generation;finished_last_frame=finished_since_present.exchange(false);if(!frame_seen())capture::set_post_effect_source(false);}
void enter_present(){presenting=true;}
void leave_present(){presenting=false;}
void mark_pre_render(){pre_rendered=generation.load();}
}
