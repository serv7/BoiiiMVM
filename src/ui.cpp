#include "backend.hpp"
#include "editor.hpp"
#include "binds.hpp"
#include "capture.hpp"
#include <imgui.h>
#include <cstdio>
#include <string>
#include <filesystem>
namespace theater {
namespace {
constexpr ImU32 amber=IM_COL32(255,181,35,255),white=IM_COL32(239,239,232,255);
constexpr ImU32 muted=IM_COL32(173,174,168,255),cyan=IM_COL32(61,189,223,255);
const char* view_name(int view) {return view==0?"1ST PERSON":view==1?"3RD PERSON":"FREE CAMERA";}
const char* mode_name(int mode) {return mode==2?"DOLLY CAMERA":mode==1?"EDIT CAMERA MODE":"FREE ROAM";}
std::string time_text(int tick,bool precise=false) {
    tick=std::max(0,tick);int seconds=tick/1000;char text[40];
    if(precise)std::snprintf(text,sizeof(text),"%02d:%02d.%03d",seconds/60,seconds%60,tick%1000);
    else std::snprintf(text,sizeof(text),"%02d:%02d",seconds/60,seconds%60);
    return text;
}
void button(const char* text,action type,int index=-1) {if(ImGui::Button(text))enqueue(type,index);}
void heading(const char* label) {ImGui::TextColored({1,.71f,.14f,1},"%s",label);ImGui::Separator();}
bool project(vec3 point,const snapshot& state,ImVec2& out) {
    constexpr float rad=.01745329252f;
    float cp=cosf(state.angles.x*rad),sp=sinf(state.angles.x*rad),cy=cosf(state.angles.y*rad),sy=sinf(state.angles.y*rad);
    vec3 f={cp*cy,cp*sy,-sp},r={-sy,cy,0},u={sp*cy,sp*sy,cp};
    vec3 delta=point-state.eye;float z=dot(delta,f);if(z<=1)return false;
    auto size=ImGui::GetIO().DisplaySize;
    float scale=size.x*.5f/std::tan(float(std::clamp(state.optics.fov,1.,170.))*.5f*rad);
    out={size.x*.5f+dot(delta,r)*scale/z,size.y*.5f-dot(delta,u)*scale/z};
    return out.x>=0 && out.y>=0 && out.x<=size.x && out.y<=size.y;
}
void transport(const snapshot& state,const options& settings,float scale) {
    auto size=ImGui::GetIO().DisplaySize;
    float width=std::min(900.f*scale,size.x-32.f*scale),height=202.f*scale;
    ImVec2 origin={(size.x-width)*.5f,size.y-height-20.f*scale};
    ImGui::SetNextWindowPos(origin);ImGui::SetNextWindowSize({width,height});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    ImGui::SetNextWindowBgAlpha(.74f);
    auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
    if(!menu_open)flags|=ImGuiWindowFlags_NoInputs;
    ImGui::Begin("Film editor transport",nullptr,flags);ImGui::SetWindowFontScale(scale);
    auto draw=ImGui::GetWindowDrawList();float font=ImGui::GetFontSize();
    auto text=[&](float x,float y,ImU32 color,const char* label) {
        draw->AddText(ImGui::GetFont(),font,{origin.x+x*scale,origin.y+y*scale},color,label);
    };
    auto center=[&](float x,float y,float cell_width,const char* label,ImU32 color=white) {
        float len=ImGui::CalcTextSize(label).x;
        draw->AddText({origin.x+x+cell_width*.5f-len*.5f,origin.y+y*scale},color,label);
    };
    auto hint=[&](float x,float y,const char* key,const char* label) {
        text(x,y,amber,key);float advance=ImGui::CalcTextSize(key).x/scale;
        text(x+advance+6,y,white,label);
    };
    float units=width/scale;
    hint(14,10,binds::label(binds::id::controls).c_str(),"Toggle Controls");hint(units-160,10,binds::label(binds::id::game_hud).c_str(),"Toggle HUD");
    if(state.view==2)hint(units*.36f,10,binds::label(binds::id::place).c_str(),state.repositioning?"Apply Camera Position":state.hovered>=0?"Edit Camera":"Insert Camera Marker");
    if(state.view==2){auto mode_keys=binds::label(binds::id::dolly)+" / "+binds::label(binds::id::edit);hint(14,38,mode_keys.c_str(),mode_name(state.free_mode));}
    else text(14,38,muted,"MODE: FILM EDITOR");
    char lens_label[96];std::snprintf(lens_label,sizeof(lens_label),"FOV %.2f   |   ROLL %+.1f   |   CAMERAS %02zu",state.optics.fov,state.optics.roll,state.cameras.size());
    float label_width=ImGui::CalcTextSize(lens_label).x;
    draw->AddText({origin.x+width-14*scale-label_width,origin.y+38*scale},white,lens_label);
    if(settings.show_help)text(14,66,muted,"Wheel Roll   Ctrl + Wheel FOV   Camera controls in Binds");
    else text(14,66,muted,"BO3 FILM EDITOR");
    char speed[32];std::snprintf(speed,sizeof(speed),"%s  %.3fx",state.paused?"PAUSED":"PLAYING",state.speed);
    float speed_width=ImGui::CalcTextSize(speed).x;
    draw->AddText({origin.x+width-14*scale-speed_width,origin.y+66*scale},amber,speed);
    float left=14*scale,right=width-78*scale,bar_width=right-left;
    const float bar_y=116*scale,bar_h=13*scale;
    static bool dragging=false;static int target=0;
    bool bounded=state.end_tick>state.start_tick;
    if(!menu_open)dragging=false;
    ImGui::SetCursorPos({left,bar_y-6*scale});ImGui::InvisibleButton("Demo scrub bar",{bar_width,bar_h+12*scale});
    if(menu_open && bounded && ImGui::IsItemActive()) {
        dragging=true;
        target=timeline_tick((ImGui::GetIO().MousePos.x-origin.x-left)/bar_width,state.start_tick,state.end_tick);
    }
    if(dragging && ImGui::IsItemDeactivated()) {enqueue(action::seek_to,target);dragging=false;}
    int shown=dragging?target:state.tick;
    float playhead=origin.x+left+bar_width*timeline_fraction(shown,state.start_tick,state.end_tick);
    draw->AddRectFilled({origin.x+left,origin.y+bar_y},{origin.x+right,origin.y+bar_y+bar_h},IM_COL32(8,8,8,255));
    draw->AddRectFilled({origin.x+left+scale,origin.y+bar_y+scale},{playhead,origin.y+bar_y+bar_h-scale},IM_COL32(95,91,67,145));
    draw->AddRect({origin.x+left,origin.y+bar_y},{origin.x+right,origin.y+bar_y+bar_h},IM_COL32(144,144,138,210));
    for(int i=0;i<=10;++i) {
        float x=origin.x+left+bar_width*i/10;
        draw->AddLine({x,origin.y+bar_y+bar_h},{x,origin.y+bar_y+bar_h-4*scale},IM_COL32(119,119,115,160));
    }
    if(bounded)for(int i=0;i<int(state.cameras.size());++i) {
        float x=origin.x+left+bar_width*timeline_fraction(state.cameras[i].tick,state.start_tick,state.end_tick);
        draw->AddLine({x,origin.y+bar_y+scale},{x,origin.y+bar_y+bar_h-scale},i==state.selected?amber:IM_COL32(101,229,107,255),2*scale);
    }
    draw->AddLine({playhead,origin.y+bar_y},{playhead,origin.y+bar_y+bar_h},white,scale);
    draw->AddTriangleFilled({playhead-5*scale,origin.y+bar_y-9*scale},{playhead+5*scale,origin.y+bar_y-9*scale},{playhead,origin.y+bar_y-2*scale},white);
    auto now=time_text(shown-state.start_tick),total=bounded?time_text(state.end_tick-state.start_tick):"--:--";
    float time_width=ImGui::CalcTextSize(now.c_str()).x;
    draw->AddText({std::clamp(playhead-time_width*.5f,origin.x+left,origin.x+right-time_width),origin.y+89*scale},white,now.c_str());
    draw->AddText({origin.x+right+9*scale,origin.y+bar_y-4*scale},white,total.c_str());
    if(menu_open && bounded && ImGui::IsItemHovered()) {
        int hover_tick=timeline_tick((ImGui::GetIO().MousePos.x-origin.x-left)/bar_width,state.start_tick,state.end_tick);
        ImGui::SetTooltip("%s  |  Drag, then release to seek",time_text(hover_tick-state.start_tick,true).c_str());
    }
    struct cell {const char* label;std::string key;action type;float weight;};
    cell cells[]={{view_name(state.view),binds::label(binds::id::view),action::cycle_view,1.8f},
        {"|<",binds::label(binds::id::seek_back),action::seek_back,.6f},{"-",binds::label(binds::id::speed_down),action::speed_down,.55f},
        {state.paused?">":"||",binds::label(binds::id::pause),action::pause,.8f},{"+",binds::label(binds::id::speed_up),action::speed_up,.55f},
        {">|",binds::label(binds::id::seek_forward),action::seek_forward,.6f},{"T7MVM",binds::label(binds::id::menu),action::toggle_ui,1.f},
        {"1ST MARKER",binds::label(binds::id::first_camera),action::first_camera,1.45f}};
    float gap=4*scale,available=width-28*scale-gap*7,x=14*scale;
    float weight=0;for(auto& cell:cells)weight+=cell.weight;
    for(int i=0;i<8;++i) {
        auto& cell=cells[i];float w=available*cell.weight/weight;
        ImGui::PushID(i);ImGui::SetCursorPos({x,141*scale});
        bool clicked=ImGui::InvisibleButton("Transport",{w,32*scale});
        bool hover=menu_open && ImGui::IsItemHovered();
        draw->AddRectFilled({origin.x+x,origin.y+141*scale},{origin.x+x+w,origin.y+173*scale},hover?IM_COL32(85,68,30,240):IM_COL32(41,41,39,220));
        draw->AddRect({origin.x+x,origin.y+141*scale},{origin.x+x+w,origin.y+173*scale},IM_COL32(78,78,72,230));
        center(x,146,w,cell.label,i==6?cyan:white);
        if(settings.show_help) {
            float hint_size=font*.7f;auto extent=ImGui::GetFont()->CalcTextSizeA(hint_size,FLT_MAX,0,cell.key.c_str());
            draw->AddText(ImGui::GetFont(),hint_size,{origin.x+x+w*.5f-extent.x*.5f,origin.y+179*scale},amber,cell.key.c_str());
        }
        if(clicked && menu_open) {if(cell.type==action::toggle_ui)menu_open=false;else enqueue(cell.type);}
        ImGui::PopID();x+=w+gap;
    }
    ImGui::End();ImGui::PopStyleVar(2);
}
void camera_editor(const snapshot& state,float scale) {
    auto size=ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x*.5f,size.y*.45f},ImGuiCond_Always,{.5f,.5f});
    ImGui::SetNextWindowSize({430*scale,220*scale});
    ImGui::Begin("Edit camera marker",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(scale);
    ImGui::Text("CAMERA %02d",state.selected+1);ImGui::Separator();
    button("Reposition",action::reposition);ImGui::SameLine();button("Delete",action::erase);
    ImGui::SameLine();button("Apply lens",action::apply);
    ImGui::TextWrapped("Reposition the camera, then press %s to apply its new position.",binds::label(binds::id::place).c_str());
    if(ImGui::Button("Close")) {close_camera_editor();menu_open=false;}
    ImGui::End();
}
}
void configure_ui() {
    ImGui::StyleColorsDark();
    if(std::filesystem::exists("C:/Windows/Fonts/bahnschrift.ttf"))
        ImGui::GetIO().Fonts->AddFontFromFileTTF("C:/Windows/Fonts/bahnschrift.ttf",20.f);
    else {ImFontConfig font; font.SizePixels=20;ImGui::GetIO().Fonts->AddFontDefault(&font);}
    auto& style=ImGui::GetStyle();
    style.WindowRounding=0;style.FrameRounding=0;style.GrabRounding=0;style.WindowBorderSize=1;
    style.Colors[ImGuiCol_WindowBg]={.045f,.045f,.045f,.94f};
    style.Colors[ImGuiCol_Button]={.16f,.16f,.16f,.85f};
    style.Colors[ImGuiCol_ButtonHovered]={.33f,.25f,.11f,1};
    style.Colors[ImGuiCol_ButtonActive]={.5f,.34f,.09f,1};
    style.Colors[ImGuiCol_SliderGrab]={1,.67f,.12f,1};
    style.Colors[ImGuiCol_SliderGrabActive]={1,.8f,.27f,1};
    style.Colors[ImGuiCol_CheckMark]={1,.67f,.12f,1};
}
void draw_ui() {
    snapshot state=current_snapshot();if(!state.playing || !hud_visible)return;
    auto recording=capture::get_status();
    if(recording.recording) {
        ImGui::GetForegroundDrawList()->AddCircleFilled({28,28},7,IM_COL32(255,48,47,255));
        char label[96];std::snprintf(label,sizeof(label),"REC %llu  %s",recording.frames,binds::label(binds::id::record).c_str());
        ImGui::GetForegroundDrawList()->AddText({42,18},IM_COL32(255,245,239,255),label);
    }
    auto settings=current_options();auto size=ImGui::GetIO().DisplaySize;float scale=std::clamp(size.y/1080.f,.6f,1.6f);
    if(settings.show_markers && state.view==2) {
        auto draw=ImGui::GetBackgroundDrawList();
        for(int i=0;i<int(state.cameras.size());++i) {
            ImVec2 p;if(!project(state.cameras[i].position,state,p))continue;bool highlight=i==state.hovered || i==state.selected;
            draw->AddCircle(p,(highlight?12.f:8.f)*scale,highlight?amber:white,16,2*scale);
            char label[72];std::snprintf(label,sizeof(label),"CAM %02d  %s",i+1,time_text(state.cameras[i].tick-state.start_tick,true).c_str());
            draw->AddText(ImGui::GetFont(),20*scale,{p.x+15*scale,p.y-8*scale},highlight?amber:white,label);
        }
    }
    if(menu_open) {if(state.editing_camera)camera_editor(state,scale);else editor::draw_menu(scale);}transport(state,settings,scale);
    if(exit_dialog){
        ImGui::SetNextWindowPos({size.x*.5f,size.y*.43f},ImGuiCond_Always,{.5f,.5f});
        ImGui::SetNextWindowSize({420*scale,0});
        ImGui::Begin("Leave theater film",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(scale);
        ImGui::TextWrapped("Exit this film and return to the main menu?");ImGui::Spacing();
        if(ImGui::Button("Return to main menu",{210*scale,34*scale})){
            exit_dialog=false;enqueue(action::exit_film);
        }
        ImGui::SameLine();if(ImGui::Button("Stay in film",{150*scale,34*scale}))exit_dialog=false;
        ImGui::TextDisabled("Escape closes this dialog. %s opens it again.",binds::label(binds::id::exit_film).c_str());
        ImGui::End();
    }
}
}
