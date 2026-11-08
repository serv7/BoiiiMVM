#include "../src/backend.hpp"
#include "../src/editor.hpp"
#include "../src/capture.hpp"
#include "../src/binds.hpp"
#include "../src/animations.hpp"
#include "../src/gif_preview.hpp"
#include "../src/vendor/json.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <dxgi.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
std::map<ImGuiID,ImRect> items;
void ImGuiTestEngineHook_ItemAdd(ImGuiContext*,const ImRect& bb,ImGuiID id){items[id]=bb;}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext*,ImGuiID,const char*,ImGuiItemStatusFlags){}
void ImGuiTestEngineHook_Log(ImGuiContext*,const char*,...){}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*,ImGuiID){return "";}
ImRect item(const char* window,const char* label,const char* nested=nullptr) {
    auto w=ImGui::FindWindowByName(window);
    if(!w)for(auto candidate:GImGui->Windows)if(std::string(candidate->Name).starts_with(std::string(window)+"_") &&
        (!w || std::strlen(candidate->Name)<std::strlen(w->Name)))w=candidate;
    assert(w);
    ImGuiID id=ImHashStr(label,0,w->ID);
    if(nested)id=ImHashStr(nested,0,id);
    if(!items.contains(id) && !nested){
        auto table_id=ImHashStr("Death swap workspace",0,w->ID);
        auto table_item=ImHashStr(label,0,table_id);
        if(items.contains(table_item))id=table_item;
    }
    if(!items.contains(id))std::cerr<<"Missing item: "<<w->Name<<" / "<<label<<" / "<<(nested?nested:"")<<'\n';
    assert(items.contains(id));return items.at(id);
}
void backdrop() {
    auto draw=ImGui::GetBackgroundDrawList();auto size=ImGui::GetIO().DisplaySize;
    draw->AddRectFilledMultiColor({0,0},size,IM_COL32(200,205,185,255),IM_COL32(120,175,200,255),IM_COL32(70,90,65,255),IM_COL32(90,78,60,255));
    // Bright/dark blocks exercise label readability in transparent mode.
    for(int y=0;y<6;++y)draw->AddRectFilled({0,size.y*y/6},{size.x*.32f,size.y*(y+1)/6},y%2?IM_COL32(36,43,47,255):IM_COL32(210,199,153,255));
}
namespace theater {
namespace animations {
snapshot data{true,{{"pb_crouch_death_clutchchest",{}},{"pb_death_headshot_front",{}},{"pb_death_swim_over_nanobots",{}}},"Choose an animation."};
std::vector<request> requests;
snapshot current(){return data;}
void enqueue(request value){requests.push_back(std::move(value));}
}
namespace capture {
settings current_settings;status current_status;
settings get_settings(){return current_settings;}
void set_settings(settings value){current_settings=value;}
status get_status(){return current_status;}
void toggle(){current_status.recording=!current_status.recording;}
}
std::atomic<bool> menu_open=false,hud_visible=true,exit_dialog=false;
snapshot demo;options settings;
bool game_menu=false;int menu_requests=0;std::vector<action> actions;
snapshot current_snapshot(){return demo;}
bool native_menu_active(){return game_menu;}
void open_game_menu(){++menu_requests;}
options current_options(){return settings;}
void set_options(options v){settings=v;}
void enqueue(action value,int){actions.push_back(value);}
void enqueue_lens(lens value){demo.optics=value;}
void close_camera_editor(){}
namespace editor {
snapshot editing;
std::vector<request> requests;
snapshot current(){return editing;}
void enqueue(request value){requests.push_back(value);if(value.type==operation::change_fog || value.type==operation::change_sun || value.type==operation::change_misc || value.type==operation::change_light)editing.values=value.values;}
}
}
int main(int argc,char** argv) {
    using namespace theater;std::cout<<std::unitbuf;
    assert(binds::match({VK_F5,0})==binds::id::record);
    assert(binds::assign(binds::id::menu,{VK_F6,0}));
    assert(binds::match({VK_F6,0})==binds::id::menu);
    binds::load();assert(binds::match({VK_F6,0})==binds::id::menu);
    assert(binds::assign(binds::id::record,{VK_F6,0}));
    assert(binds::match({VK_F6,0})==binds::id::record && binds::match({VK_F5,0})==binds::id::menu);
    binds::defaults();assert(binds::match({VK_F5,0})==binds::id::record);
    assert(binds::match({VK_OEM_MINUS,binds::shift})==binds::id::speed_minimum);
    assert(binds::match({'0',0})==binds::id::speed_normal);
    assert(binds::match({VK_OEM_PLUS,binds::shift})==binds::id::speed_maximum);
    nlohmann::json legacy;{std::ifstream in("MVM/Theater/binds.json");in>>legacy;}
    while(legacy["binds"].size()>22)legacy["binds"].erase(legacy["binds"].end()-1);
    legacy["binds"][0]={{"key",VK_OEM_PLUS},{"modifiers",binds::shift}};
    {std::ofstream out("MVM/Theater/binds.json");out<<legacy;}
    binds::load();assert(binds::match({VK_OEM_PLUS,binds::shift})==binds::id::menu);
    assert(binds::get(binds::id::speed_maximum).key!=VK_OEM_PLUS);binds::defaults();
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;
    bool hardware=argc>1 && std::string(argv[1])=="--hardware";
    auto driver=hardware?D3D_DRIVER_TYPE_HARDWARE:D3D_DRIVER_TYPE_WARP;
    auto created=D3D11CreateDevice(nullptr,driver,nullptr,D3D11_CREATE_DEVICE_DEBUG,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
    if(FAILED(created))created=D3D11CreateDevice(nullptr,driver,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
    assert(SUCCEEDED(created));
    IDXGIDevice* dxgi=nullptr;assert(SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice),reinterpret_cast<void**>(&dxgi))));
    IDXGIAdapter* adapter=nullptr;dxgi->GetAdapter(&adapter);DXGI_ADAPTER_DESC adapter_desc{};adapter->GetDesc(&adapter_desc);
    std::wcout<<L"Preview adapter: "<<adapter_desc.Description<<L'\n';adapter->Release();dxgi->Release();
    ImGui::CreateContext();GImGui->TestEngineHookItems=true;configure_ui();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.f/60;
    ImGui_ImplDX11_Init(device,context);
    gif_preview::set_device(device);gif_preview::set_directory("assets/death-previews");
    auto wait_preview=[](const std::string& name){
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        ID3D11ShaderResourceView* result=nullptr;
        while(!(result=gif_preview::frame(name)) && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        assert(result);return result;
    };
    int previews=0;
    for(const auto& file:std::filesystem::directory_iterator("assets/death-previews"))if(file.path().extension()==".gif") {
        auto image=wait_preview(file.path().stem().string());
        ID3D11Resource* resource=nullptr;image->GetResource(&resource);ID3D11Texture2D* texture=nullptr;
        assert(SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture))));
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);assert(desc.Usage==D3D11_USAGE_IMMUTABLE && desc.CPUAccessFlags==0);
        texture->Release();resource->Release();gif_preview::finish_frame();++previews;
    }
    assert(previews==70);std::cout<<"All 70 GIF previews decoded in background and uploaded as immutable images.\n";
    // Retain a queued image while another request replaces the single selection.
    // The extra test reference makes the lifetime assertion safe even if broken.
    D3D11_TEXTURE2D_DESC stress_desc{};stress_desc.Width=1280;stress_desc.Height=720;stress_desc.MipLevels=stress_desc.ArraySize=1;
    stress_desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;stress_desc.SampleDesc.Count=1;stress_desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* stress_texture=nullptr;ID3D11RenderTargetView* stress_target=nullptr;
    assert(SUCCEEDED(device->CreateTexture2D(&stress_desc,nullptr,&stress_texture)));
    assert(SUCCEEDED(device->CreateRenderTargetView(stress_texture,nullptr,&stress_target)));
    context->OMSetRenderTargets(1,&stress_target,nullptr);
    auto queued=wait_preview("pb_death_run_forward_crumple");queued->AddRef();
    gif_preview::frame("pb_death_run_stumble");
    assert(queued->Release()>=1); // ImGui's queued command still owns a frame pin.
    auto other=wait_preview("pb_death_run_stumble");
    ImGui_ImplDX11_NewFrame();io.DisplaySize={1280,720};ImGui::NewFrame();
    ImGui::Begin("Two queued GIFs");ImGui::Image(queued,{192,192});ImGui::Image(other,{192,192});ImGui::End();
    ImGui::Render();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());gif_preview::finish_frame();
    for(int cycle=0;cycle<60;++cycle) {
        // Closing/reopening, two selections before submission, and animated
        // frame replacement all use the same lifetime boundary as the plugin.
        ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
        if(cycle%5) {
            auto first=wait_preview("pb_death_run_forward_crumple");
            auto second=wait_preview("pb_death_run_stumble");
            ImGui::Begin("Reopened preview panel");ImGui::Image(first,{192,192});ImGui::Image(second,{192,192});ImGui::End();
        }
        ImGui::Render();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());gif_preview::finish_frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    context->OMSetRenderTargets(0,nullptr,nullptr);stress_target->Release();stress_texture->Release();
    assert(!gif_preview::frame("../outside"));gif_preview::finish_frame();
    std::cout<<"Queued image lifetime and 60 preview reopen/switch cycles passed on "<<(hardware?"hardware":"WARP")<<".\n";
    animations::data.active={{"pb_crouch_death_clutchchest",18,.2f,1},{"pb_death_headshot_front",19,.4f,1}};
    animations::data.active_tick=22000;animations::data.active_scans=1;
    animations::data.catalog=std::make_shared<const std::vector<animations::entry>>(animations::data.entries);
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
    for(bool transparent:{false,true})for(auto [width,height]:{std::pair{1280,720},std::pair{1920,1080},std::pair{2560,1440}})for(int panel=0;panel<13;++panel) {
        ImGui::ClosePopupsOverWindow(nullptr,false);io.AddMousePosEvent(-1,-1);io.AddMouseButtonEvent(0,false);
        settings.menu_background=!transparent;editor::select_section(panel==11?2:std::max(0,panel-1));
        demo.editing_camera=panel==10;exit_dialog=panel==12;
        menu_open=panel!=0 && panel!=12;io.DisplaySize={float(width),float(height)};
        D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
        ID3D11Texture2D* texture=nullptr;ID3D11RenderTargetView* target=nullptr;ID3D11Texture2D* staging=nullptr;
        assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&texture)));assert(SUCCEEDED(device->CreateRenderTargetView(texture,nullptr,&target)));
        bool preview_drawn=false;
        for(int frame=0;frame<6 || (panel==9 && !preview_drawn && frame<120);++frame) {
            if(panel==9 && frame>=1 && frame<=3) {
                ImGuiWindow* list=nullptr;for(auto w:GImGui->Windows)if(w->Active && std::string(w->Name).find("/Active death list_")!=std::string::npos){list=w;break;}
                assert(list);auto r=items.at(ImHashStr("pb_crouch_death_clutchchest##18",0,list->ID));
                io.AddMousePosEvent(r.GetCenter().x,r.GetCenter().y);
                if(frame==2)io.AddMouseButtonEvent(0,true);
                if(frame==3)io.AddMouseButtonEvent(0,false);
            }
            if(panel==9 && frame==4)io.AddMousePosEvent(-1,-1);
            if(panel==11 && frame>=1 && frame<=3) {
                auto r=item("T7MVM scene editor/Settings content","Sun color","##swatch");
                io.AddMousePosEvent(r.GetCenter().x,r.GetCenter().y);
                if(frame==2)io.AddMouseButtonEvent(0,true);
                if(frame==3)io.AddMouseButtonEvent(0,false);
            }
            float clear[]={.15f,.18f,.19f,1};context->ClearRenderTargetView(target,clear);context->OMSetRenderTargets(1,&target,nullptr);
            ImGui_ImplDX11_NewFrame();ImGui::NewFrame();backdrop();draw_ui();ImGui::Render();
            if(panel==9)for(int list_index=0;list_index<ImGui::GetDrawData()->CmdListsCount;++list_index)
                for(const auto& command:ImGui::GetDrawData()->CmdLists[list_index]->CmdBuffer)
                    if(command.ElemCount && command.GetTexID() && command.GetTexID()!=io.Fonts->TexID)preview_drawn=true;
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());gif_preview::finish_frame();
            if(panel==9 && !preview_drawn)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(panel==9)assert(preview_drawn && "The GIF must be visible without scrolling at every supported resolution");
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)));context->CopyResource(staging,texture);
        D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped)));
        const char* names[]={"hud","sliders","fog","sun","lights","misc","configs","settings","binds","animations","camera-edit","color-popup","escape-menu"};
        std::string name="build/previews/"+std::to_string(height)+(transparent?"-transparent-":"-")+names[panel]+".ppm";
        std::ofstream out(name,std::ios::binary);out<<"P6\n"<<width<<' '<<height<<"\n255\n";
        for(int y=0;y<height;++y)for(int x=0;x<width;++x)out.write(reinterpret_cast<char*>(mapped.pData)+y*mapped.RowPitch+x*4,3);
        context->Unmap(staging,0);staging->Release();target->Release();texture->Release();std::cout<<name<<'\n';
    }
    io.DisplaySize={1920,1080};menu_open=true;exit_dialog=false;demo.editing_camera=false;editor::select_section(0);settings.menu_background=true;
    auto frame=[] {ImGui::NewFrame();backdrop();draw_ui();ImGui::Render();};
    auto click=[&](float x,float y) {io.AddMousePosEvent(x,y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();};
    auto click_item=[&](const char* window,const char* label,const char* nested=nullptr,float fraction=.5f) {
        auto r=item(window,label,nested);click(r.Min.x+r.GetWidth()*fraction,r.GetCenter().y);
    };
    frame();frame();
    const char* content="T7MVM scene editor/Settings content";
    click_item(content,"Movement speed","##value",.2f);assert(settings.move_speed<.01f);
    click_item(content,"Shift multiplier","##value",.7f);assert(settings.fast_multiplier>5);
    click_item(content,"Ctrl multiplier","##value",.3f);assert(settings.slow_multiplier<.1f);
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
    editor::select_section(8);frame();frame();
    animations::requests.clear();click_item(content,"Scan active deaths");
    assert(animations::requests.size()==1 && animations::requests[0].type==animations::operation::inspect_active && animations::requests[0].tick==demo.tick);
    animations::requests.clear();for(int i=0;i<20;++i)frame();assert(animations::requests.empty());
    auto clip_list=[&]()->ImGuiWindow*{for(auto w:GImGui->Windows)if(std::string(w->Name).find("/Death clip list_")!=std::string::npos && w->Active)return w;return nullptr;};
    items.clear();frame();frame();assert(clip_list());
    assert(!items.contains(ImHashStr("pb_death_swim_over_nanobots",0,clip_list()->ID)));
    click_item(content,"Show BO3 animations");frame();
    assert(items.contains(ImHashStr("pb_death_swim_over_nanobots",0,clip_list()->ID)));
    click_item(content,"Show BO2 / GIF animations");
    auto previous_hold=settings.hold_weapons;click_item(content,"Hold gun in demo");assert(settings.hold_weapons!=previous_hold);
    auto choose_clip=[&](const char* name) {
        ImGuiWindow* list=nullptr;for(auto w:GImGui->Windows)if(std::string(w->Name).find("/Death clip list_")!=std::string::npos && w->Active){list=w;break;}
        assert(list);auto rect=items.at(ImHashStr(name,0,list->ID));click(rect.GetCenter().x,rect.GetCenter().y);
    };
    ImGuiWindow* active_list=nullptr;for(auto w:GImGui->Windows)if(w->Active && std::string(w->Name).find("/Active death list_")!=std::string::npos){active_list=w;break;}
    assert(active_list);auto active_row=items.at(ImHashStr("pb_crouch_death_clutchchest##18",0,active_list->ID));click(active_row.GetCenter().x,active_row.GetCenter().y);
    choose_clip("pb_death_headshot_front");
    wait_preview("pb_death_headshot_front");gif_preview::finish_frame();
    auto verify_single_preview_hover=[&](const ImRect& row) {
        io.AddMousePosEvent(row.GetCenter().x,row.GetCenter().y);
        for(int i=0;i<6;++i) {
            frame();
            for(auto window:GImGui->Windows)
                assert(!(window->Active && !window->Hidden && (window->Flags&ImGuiWindowFlags_Tooltip)) && "Animation rows must not open hover previews");
            int images=0;
            for(int list_index=0;list_index<ImGui::GetDrawData()->CmdListsCount;++list_index)
                for(const auto& command:ImGui::GetDrawData()->CmdLists[list_index]->CmdBuffer)
                    if(command.ElemCount && command.GetTexID() && command.GetTexID()!=io.Fonts->TexID)++images;
            assert(images==1 && "Only the fixed preview panel may draw an animation image");
            // Switching the decoder selection returns null until its first frame
            // arrives. A hover must leave the selected replacement GIF loaded.
            assert(gif_preview::frame("pb_death_headshot_front") && "Hovering must not change the selected preview");
            gif_preview::finish_frame();
        }
    };
    verify_single_preview_hover(active_row);
    verify_single_preview_hover(items.at(ImHashStr("pb_crouch_death_clutchchest",0,clip_list()->ID)));
    animations::requests.clear();click_item(content,"Replace");
    assert(animations::requests.size()==1 && animations::requests[0].target=="pb_crouch_death_clutchchest" && animations::requests[0].source=="pb_death_headshot_front");
    demo.paused=false;frame();frame();animations::requests.clear();click_item(content,"Replace");click_item(content,"Scan active deaths");assert(animations::requests.empty());demo.paused=true;
    // Every horizontal tab fits inside the docked panel at every target size.
    for(auto [width,height]:{std::pair{1280,720},std::pair{1920,1080},std::pair{2560,1440}}) {
        io.DisplaySize={float(width),float(height)};frame();frame();auto w=ImGui::FindWindowByName("T7MVM scene editor");
        assert(w->Pos.x<width*.02f && w->Size.x<width*.4f && w->Pos.y+w->Size.y<height*.8f);
        auto restore_rect=item(content,"Restore all");
        auto content_window=ImGui::FindWindowByName("T7MVM scene editor/Settings content_FDDD353D");assert(content_window);
        assert(restore_rect.Max.y<=content_window->InnerRect.Max.y && restore_rect.Max.x<w->Pos.x+w->Size.x-8);
        const char* tabs[]={"Sliders","Fog","Sun","Lights","Misc","Configs","Settings","Binds","Animations"};
        for(int i=0;i<9;++i) {
            ImGuiID seed=ImHashData(&i,sizeof(i),w->ID);auto r=items.at(ImHashStr(tabs[i],0,seed));
            assert(r.Min.x>=w->Pos.x && r.Max.x<=w->Pos.x+w->Size.x-8 && r.GetHeight()>10);
        }
    }
    menu_open=false;exit_dialog=true;io.DisplaySize={1920,1080};frame();frame();
    click_item("Theater menu","Resume");assert(!exit_dialog);
    exit_dialog=true;frame();frame();click_item("Theater menu","Game menu / settings");assert(!exit_dialog && menu_requests==1);
    exit_dialog=true;frame();frame();actions.clear();click_item("Theater menu","Exit demo");assert(!exit_dialog && actions.size()==1 && actions[0]==action::exit_film);
    game_menu=true;frame();assert(!ImGui::FindWindowByName("Film editor transport")->Active);
    std::cout<<"UI interaction checks passed: dock size, all tabs, saved binds, camera modifiers, movement/FOV/fog sliders, useful dvars, color wheel, animation selection, single selection-driven preview without hover tooltips, paused-swap guard, Escape choices and background toggle at 720p/1080p/1440p.\n";
    ID3D11InfoQueue* messages=nullptr;
    if(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11InfoQueue),reinterpret_cast<void**>(&messages)))) {
        for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){
            SIZE_T bytes=0;messages->GetMessage(i,nullptr,&bytes);std::vector<unsigned char> storage(bytes);
            auto message=reinterpret_cast<D3D11_MESSAGE*>(storage.data());messages->GetMessage(i,message,&bytes);
            if(message->Severity<=D3D11_MESSAGE_SEVERITY_ERROR){std::cerr<<message->pDescription<<'\n';assert(false && "D3D11 validation error");}
        }
        std::cout<<"D3D11 debug layer: no corruption/errors.\n";messages->Release();
    }else std::cout<<"D3D11 debug layer unavailable.\n";
    gif_preview::shutdown();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();context->Release();device->Release();
}
