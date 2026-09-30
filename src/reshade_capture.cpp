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
std::atomic<unsigned long long> pre_rendered=~0ull,last_frame_ms=0;

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
        bool already_drawn=pre_rendered.load()==frame;
        bool first=!seen_frame.exchange(true);
        if(first)log_message("ReShade finish-effects callback received; clean capture begins next frame");
        last_frame_ms=GetTickCount64();
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
bool frame_seen(){return seen_frame && GetTickCount64()-last_frame_ms.load()<2000;}
void begin_present(){++generation;if(!frame_seen())capture::set_post_effect_source(false);}
void mark_pre_render(){pre_rendered=generation.load();}
}
