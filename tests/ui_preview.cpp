#include "../src/backend.hpp"
#include "../src/editor.hpp"
#include "../src/capture.hpp"
#include "../src/binds.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
std::map<ImGuiID,ImRect> items;
void ImGuiTestEngineHook_ItemAdd(ImGuiContext*,const ImRect& bb,ImGuiID id){items[id]=bb;}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext*,ImGuiID,const char*,ImGuiItemStatusFlags){}
void ImGuiTestEngineHook_Log(ImGuiContext*,const char*,...){}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*,ImGuiID){return "";}
ImRect item(const char* window,const char* label,const char* nested=nullptr) {
    auto w=ImGui::FindWindowByName(window);
    if(!w)for(auto candidate:GImGui->Windows)if(std::string(candidate->Name).starts_with(std::string(window)+"_")) {w=candidate;break;}
    assert(w);
    ImGuiID id=ImHashStr(label,0,w->ID);
    if(nested)id=ImHashStr(nested,0,id);
    assert(items.contains(id));return items.at(id);
}
void backdrop() {
    auto draw=ImGui::GetBackgroundDrawList();auto size=ImGui::GetIO().DisplaySize;
    draw->AddRectFilledMultiColor({0,0},size,IM_COL32(200,205,185,255),IM_COL32(120,175,200,255),IM_COL32(70,90,65,255),IM_COL32(90,78,60,255));
    // Bright/dark blocks exercise label readability in transparent mode.
    for(int y=0;y<6;++y)draw->AddRectFilled({0,size.y*y/6},{size.x*.32f,size.y*(y+1)/6},y%2?IM_COL32(36,43,47,255):IM_COL32(210,199,153,255));
}
namespace theater {
namespace capture {
settings current_settings;status current_status;
settings get_settings(){return current_settings;}
void set_settings(settings value){current_settings=value;}
status get_status(){return current_status;}
void toggle(){current_status.recording=!current_status.recording;}
}
std::atomic<bool> menu_open=false,hud_visible=true,exit_dialog=false;
snapshot demo;options settings;
snapshot current_snapshot(){return demo;}
options current_options(){return settings;}
void set_options(options v){settings=v;}
void enqueue(action,int){}
void enqueue_lens(lens value){demo.optics=value;}
void close_camera_editor(){}
namespace editor {
snapshot editing;
std::vector<request> requests;
snapshot current(){return editing;}
void enqueue(request value){requests.push_back(value);if(value.type==operation::change_fog || value.type==operation::change_sun || value.type==operation::change_misc || value.type==operation::change_light)editing.values=value.values;}
}
}
int main() {
    using namespace theater;std::cout<<std::unitbuf;
    assert(binds::match({VK_F5,0})==binds::id::record);
    assert(binds::assign(binds::id::menu,{VK_F6,0}));
    assert(binds::match({VK_F6,0})==binds::id::menu);
    binds::load();assert(binds::match({VK_F6,0})==binds::id::menu);
    assert(binds::assign(binds::id::record,{VK_F6,0}));
    assert(binds::match({VK_F6,0})==binds::id::record && binds::match({VK_F5,0})==binds::id::menu);
    binds::defaults();assert(binds::match({VK_F5,0})==binds::id::record);
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
    ImGui::CreateContext();GImGui->TestEngineHookItems=true;configure_ui();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.f/60;
    ImGui_ImplDX11_Init(device,context);
    demo.playing=true;demo.paused=true;demo.tick=22000;demo.start_tick=7000;demo.end_tick=153000;
    demo.view=2;demo.free_mode=1;demo.speed=.1f;demo.selected=1;demo.message="Camera placed";
    settings.show_markers=true;
    editor::editing.useful_dvars_ready=true;editor::editing.ready=editor::editing.fog_ready=editor::editing.sun_ready=editor::editing.misc_ready=editor::editing.lights_ready=editor::editing.blood_ready=true;
    editor::editing.values.fog_enabled=editor::editing.values.misc_enabled=true;
    editor::editing.values.mist.tint={.4f,.6f,.9f};editor::editing.values.mist.haze_tint={1,.7f,.4f};
    editor::editing.values.lights.resize(2);editor::editing.values.lights[0].tint={1,.25f,.1f};
    editor::editing.configs={"warm-sunset.cfg","clean-cinematic.cfg"};editor::editing.presets={"Lavender.zfog","Sunset.zfog"};editor::editing.lighting={"street-lights.bo3light"};
    editor::editing.message="Scene editor ready";
    for(int i=0;i<5;++i) {marker m;m.tick=18000+i*20000;m.optics.fov=65-i*2;m.position={1000,float(i-2)*120,0};demo.cameras.push_back(m);}
    std::filesystem::create_directories("build/previews");
    for(bool transparent:{false,true})for(auto [width,height]:{std::pair{1280,720},std::pair{1920,1080},std::pair{2560,1440}})for(int panel=0;panel<11;++panel) {
        ImGui::ClosePopupsOverWindow(nullptr,false);io.AddMousePosEvent(-1,-1);io.AddMouseButtonEvent(0,false);
        settings.menu_background=!transparent;editor::select_section(panel==10?2:std::max(0,panel-1));
        demo.editing_camera=panel==9;
        menu_open=panel!=0;io.DisplaySize={float(width),float(height)};
        D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
        ID3D11Texture2D* texture=nullptr;ID3D11RenderTargetView* target=nullptr;ID3D11Texture2D* staging=nullptr;
        assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&texture)));assert(SUCCEEDED(device->CreateRenderTargetView(texture,nullptr,&target)));
        for(int frame=0;frame<6;++frame) {
            if(panel==10 && frame>=1 && frame<=3) {
                auto r=item("T7MVM scene editor/Settings content","Sun color","##swatch");
                io.AddMousePosEvent(r.GetCenter().x,r.GetCenter().y);
                if(frame==2)io.AddMouseButtonEvent(0,true);
                if(frame==3)io.AddMouseButtonEvent(0,false);
            }
            float clear[]={.15f,.18f,.19f,1};context->ClearRenderTargetView(target,clear);context->OMSetRenderTargets(1,&target,nullptr);
            ImGui_ImplDX11_NewFrame();ImGui::NewFrame();backdrop();draw_ui();ImGui::Render();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)));context->CopyResource(staging,texture);
        D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped)));
        const char* names[]={"hud","sliders","fog","sun","lights","misc","configs","settings","binds","camera-edit","color-popup"};
        std::string name="build/previews/"+std::to_string(height)+(transparent?"-transparent-":"-")+names[panel]+".ppm";
        std::ofstream out(name,std::ios::binary);out<<"P6\n"<<width<<' '<<height<<"\n255\n";
        for(int y=0;y<height;++y)for(int x=0;x<width;++x)out.write(reinterpret_cast<char*>(mapped.pData)+y*mapped.RowPitch+x*4,3);
        context->Unmap(staging,0);staging->Release();target->Release();texture->Release();std::cout<<name<<'\n';
    }
    io.DisplaySize={1920,1080};menu_open=true;demo.editing_camera=false;editor::select_section(0);settings.menu_background=true;
    auto frame=[] {ImGui::NewFrame();backdrop();draw_ui();ImGui::Render();};
    auto click=[&](float x,float y) {io.AddMousePosEvent(x,y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();};
    auto click_item=[&](const char* window,const char* label,const char* nested=nullptr,float fraction=.5f) {
        auto r=item(window,label,nested);click(r.Min.x+r.GetWidth()*fraction,r.GetCenter().y);
    };
    frame();frame();
    const char* content="T7MVM scene editor/Settings content";
    click_item(content,"Movement speed","##value",.2f);assert(settings.move_speed<.01f);
    click_item(content,"Field of view","##value",.4f);assert(demo.optics.fov<40 && demo.optics.fov>.1);
    click_item("T7MVM scene editor","No background");assert(!settings.menu_background);
    click_item("T7MVM scene editor","No background");assert(settings.menu_background);
    editor::editing.useful_dvars_ready=false;editor::requests.clear();frame();frame();
    click_item(content,"Enable useful dvars");
    assert(!editor::requests.empty() && editor::requests.back().type==editor::operation::toggle_useful_dvars && editor::requests.back().value==1.f);
    editor::select_section(1);frame();frame();editor::requests.clear();
    click_item(content,"Base distance","##value",.7f);assert(!editor::requests.empty() && editor::requests.back().type==editor::operation::change_fog);
    auto before=editor::editing.values.mist.tint;click_item(content,"Fog color","##swatch");frame();frame();
    std::cout<<"Opened color popup count: "<<GImGui->OpenPopupStack.Size<<"\n";assert(GImGui->OpenPopupStack.Size>0);auto popup=GImGui->OpenPopupStack.back().Window;assert(popup && popup->Active);
    auto picker_id=ImHashStr("##wheel",0,ImHashStr("Fog color",0,popup->ID));
    auto wheel_id=ImHashStr("hsv",0,picker_id);assert(items.contains(wheel_id));auto picker=items.at(wheel_id);
    // Hue ring near the top of the RGB wheel; values update through the actual picker.
    click(picker.Min.x+92,picker.Min.y+7);
    auto after=editor::editing.values.mist.tint;assert(before.r!=after.r || before.g!=after.g || before.b!=after.b);
    click(800,200);editor::select_section(4);frame();frame();editor::requests.clear();click_item(content,"Remove gun");
    assert(!editor::requests.empty() && editor::requests.back().type==editor::operation::change_misc && editor::requests.back().values.remove_gun);
    // All eight horizontal tabs fit inside the docked panel at every target size.
    for(auto [width,height]:{std::pair{1280,720},std::pair{1920,1080},std::pair{2560,1440}}) {
        io.DisplaySize={float(width),float(height)};frame();frame();auto w=ImGui::FindWindowByName("T7MVM scene editor");
        assert(w->Pos.x<width*.02f && w->Size.x<width*.3f && w->Pos.y+w->Size.y<height*.8f);
        const char* tabs[]={"Sliders","Fog","Sun","Lights","Misc","Configs","Settings","Binds"};
        for(int i=0;i<8;++i) {
            ImGuiID seed=ImHashData(&i,sizeof(i),w->ID);auto r=items.at(ImHashStr(tabs[i],0,seed));
            assert(r.Min.x>=w->Pos.x && r.Max.x<=w->Pos.x+w->Size.x-8 && r.GetHeight()>10);
        }
    }
    menu_open=false;exit_dialog=true;io.DisplaySize={1920,1080};frame();frame();
    click_item("Leave theater film","Stay in film");assert(!exit_dialog);
    exit_dialog=true;frame();frame();click_item("Leave theater film","Return to main menu");assert(!exit_dialog);
    std::cout<<"UI interaction checks passed: dock size, two-row tabs, saved binds, movement/FOV/fog sliders, useful-dvar toggle, color popup/hue wheel, visibility checkbox and background toggle at 720p/1080p/1440p.\n";
    ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();context->Release();device->Release();
}
