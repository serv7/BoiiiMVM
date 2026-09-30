#include "editor.hpp"
#include "backend.hpp"
#include "binds.hpp"
#include "capture.hpp"
#include <imgui.h>
#include <cstdio>
#include <map>
namespace theater::editor {
namespace {
int section=0,selected_light=0;
bool transparent_menu=false;
const char* sections[]={"Sliders","Fog","Sun","Lights","Misc","Configs","Settings","Binds"};
void send(operation op,scene value={},int index=-1,const char* name="") {enqueue({op,std::move(value),index,name});}
// Small shadows keep unframed labels legible against both bright and dark scenes.
void shadow(ImVec2 pos,const char* label,float wrap=0) {
    if(!transparent_menu)return;auto draw=ImGui::GetWindowDrawList();
    float step=std::max(.8f,ImGui::GetFontSize()/24.f);
    for(auto offset:{ImVec2{-step,0},ImVec2{step,0},ImVec2{0,-step},ImVec2{0,step}})
        draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{pos.x+offset.x,pos.y+offset.y},IM_COL32(0,0,0,210),label,nullptr,wrap);
}
void text(const char* label,ImVec4 color={.94f,.94f,.91f,1}) {
    ImGui::TextColored(color,"%s",label);
    if(transparent_menu) {auto pos=ImGui::GetItemRectMin();shadow(pos,label);ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,ImGui::GetColorU32(color),label);}
}
void wrapped(const char* label) {
    float width=ImGui::GetContentRegionAvail().x;ImGui::TextWrapped("%s",label);
    if(transparent_menu) {auto pos=ImGui::GetItemRectMin();shadow(pos,label,width);ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,ImGui::GetColorU32(ImGuiCol_Text),label,nullptr,width);}
}
void title(const char* label) {text(label,{.24f,.74f,.87f,1});ImGui::Spacing();}
void hint(const char* label) {text(label,transparent_menu?ImVec4{.79f,.81f,.83f,1}:ImVec4{.55f,.57f,.58f,1});}
bool checkbox(const char* label,bool* value) {
    auto pos=ImGui::GetCursorScreenPos();pos.x+=ImGui::GetFrameHeight()+ImGui::GetStyle().ItemInnerSpacing.x;pos.y+=ImGui::GetStyle().FramePadding.y;
    bool changed=ImGui::Checkbox(label,value);
    if(transparent_menu) {shadow(pos,label);ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,ImGui::GetColorU32(ImGuiCol_Text),label);}
    return changed;
}
bool radio(const char* label,int* value,int choice) {
    bool changed=ImGui::RadioButton(label,value,choice);
    if(transparent_menu) {
        auto pos=ImGui::GetItemRectMin();pos.x+=ImGui::GetFrameHeight()+ImGui::GetStyle().ItemInnerSpacing.x;pos.y+=ImGui::GetStyle().FramePadding.y;
        shadow(pos,label);ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,ImGui::GetColorU32(ImGuiCol_Text),label);
    }
    return changed;
}
bool slider(const char* label,float& value,float low,float high,const char* format="%.2f",bool logarithmic=false) {
    ImGui::PushID(label);
    float width=ImGui::GetContentRegionAvail().x;
    text(label);ImGui::SameLine(width*.45f);
    ImGui::SetNextItemWidth(-1);
    bool changed=ImGui::SliderFloat("##value",&value,low,high,format,ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_AlwaysClamp|(logarithmic?ImGuiSliderFlags_Logarithmic:0));
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Ctrl + click the value to type an exact number.");
    ImGui::PopID();return changed;
}
bool color_wheel(const char* label,color& value,float scale) {
    ImGui::PushID(label);title(label);
    struct picker {color last{-1,-1,-1};float intensity=1,rgb[3]={1,1,1};};
    static std::map<ImGuiID,picker> pickers;auto& p=pickers[ImGui::GetID("##color state")];
    if(value.r!=p.last.r || value.g!=p.last.g || value.b!=p.last.b) {
        p.intensity=std::max({value.r,value.g,value.b,.000001f});p.rgb[0]=value.r/p.intensity;p.rgb[1]=value.g/p.intensity;p.rgb[2]=value.b/p.intensity;p.last=value;
        if(value.r==0 && value.g==0 && value.b==0) {p.intensity=1;p.rgb[0]=p.rgb[1]=p.rgb[2]=0;}
    }
    ImGui::SetNextItemWidth(std::min(185.f*scale,ImGui::GetContentRegionAvail().x));
    bool changed=ImGui::ColorPicker3("##wheel",p.rgb,ImGuiColorEditFlags_PickerHueWheel|ImGuiColorEditFlags_NoSidePreview|ImGuiColorEditFlags_NoSmallPreview|ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_DisplayRGB|ImGuiColorEditFlags_Float);
    ImGui::SetNextItemWidth(-1);
    changed|=ImGui::DragFloat("##hdr",&p.intensity,.01f,0,10000,"Intensity %.3f",ImGuiSliderFlags_AlwaysClamp);
    if(ImGui::IsItemHovered())ImGui::SetTooltip("HDR color strength. 1.0 is normal; values above 1 brighten the color.");
    if(changed) {value={p.rgb[0]*p.intensity,p.rgb[1]*p.intensity,p.rgb[2]*p.intensity};p.last=value;}
    ImGui::PopID();return changed;
}
bool color_control(const char* label,color& value,float scale) {
    ImGui::PushID(label);float strength=std::max({value.r,value.g,value.b,1.f});
    bool changed=false;
    if(ImGui::ColorButton("##swatch",{value.r/strength,value.g/strength,value.b/strength,1},
            ImGuiColorEditFlags_NoTooltip,{34*scale,24*scale}))ImGui::OpenPopup("Color wheel");
    ImGui::SameLine();text(label);ImGui::SameLine(ImGui::GetContentRegionAvail().x+ImGui::GetCursorPosX()-ImGui::CalcTextSize("Edit color").x-2*ImGui::GetStyle().FramePadding.x);
    if(ImGui::SmallButton("Edit color"))ImGui::OpenPopup("Color wheel");
    // Popups keep their own backing even when the main panel is transparent.
    ImGui::SetNextWindowSize({250*scale,0});
    if(ImGui::BeginPopup("Color wheel")) {
        ImGui::SetWindowFontScale(scale);
        changed=color_wheel(label,value,scale);ImGui::EndPopup();
    }
    ImGui::PopID();return changed;
}
void file_controls(const char* id,const std::vector<std::string>& files,operation save,operation load,const char* description,float scale) {
    ImGui::PushID(id);
    // Each section keeps an independent filename, so switching tabs cannot
    // overwrite another section's selection.
    static char names[3][160]={"scene.cfg","fog.zfog","lighting.bo3light"};
    int index=save==operation::save_cfg?0:save==operation::save_fog?1:2;char* name=names[index];
    title(description);ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("##files","Choose a saved file...")) {
        if(files.empty())hint("No saved files yet");
        for(auto& file:files)if(ImGui::Selectable(file.c_str(),file==name))std::snprintf(name,160,"%s",file.c_str());
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(-1);ImGui::InputText("##filename",name,160);
    float width=(ImGui::GetContentRegionAvail().x-16*scale)/3;
    if(ImGui::Button("Save",{width,30*scale}))send(save,{},-1,name);ImGui::SameLine();
    if(ImGui::Button("Load",{width,30*scale}))send(load,{},-1,name);ImGui::SameLine();
    if(ImGui::Button("Refresh",{width,30*scale}))send(operation::refresh);
    ImGui::PopID();
}
void sliders(const theater::snapshot& film,options settings,const snapshot& editor) {
    title("CAMERA SLIDERS");
    if(slider("Movement speed",settings.move_speed,.0001f,10,"%.4fx",true))set_options(settings);
    hint("Q down / E up use the same speed.");
    ImGui::Spacing();lens optics=film.optics;float fov=float(optics.fov);
    if(slider("Field of view",fov,.1f,75,"%.2f")) {optics.fov=fov;enqueue_lens(optics);}
    hint("Ctrl + mouse wheel adjusts FOV.");ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button("Reset movement to 1x")) {settings.move_speed=1;set_options(settings);}
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button(editor.useful_dvars?"Disable useful dvars":"Enable useful dvars",{-1,30*ImGui::GetFontSize()/20.f}))
        enqueue({operation::toggle_useful_dvars,{},-1,"",editor.useful_dvars?0.f:1.f});
    hint(editor.useful_dvars?"Shadow and dolly settings enabled":"Restores original values when turned off.");
    if(!editor.useful_dvars_ready)hint("Click to rescan and apply; unavailable settings are reported below.");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button("Exit film to main menu",{-1,30*ImGui::GetFontSize()/20.f})){
        menu_open=false;exit_dialog=true;
    }
    ImGui::Spacing();wrapped("Drag sliders or Ctrl + click their values for precise input. Movement speed reaches 0.0001x.");
}
void fog_section(snapshot& s,float scale) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{6*scale,2*scale});ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6*scale,4*scale});
    bool changed=checkbox("Enable fog override",&s.values.fog_enabled);
    if(!s.fog_ready) {hint("Waiting for the film's fog data...");ImGui::PopStyleVar(2);return;}
    auto& f=s.values.mist;ImGui::BeginDisabled(!s.values.fog_enabled);
    changed|=color_control("Fog color",f.tint,scale);
    ImGui::BeginDisabled(s.values.sync_colors);changed|=color_control("Sun haze color",f.haze_tint,scale);ImGui::EndDisabled();
    changed|=checkbox("Sync colors",&s.values.sync_colors);ImGui::SameLine();changed|=checkbox("Sync sun to fog",&s.values.sync_sun);
    if(ImGui::CollapsingHeader("Distance & height",ImGuiTreeNodeFlags_DefaultOpen)) {
        changed|=slider("Base distance",f.base_distance,-200,5000);changed|=slider("Half distance",f.half_distance,0,17500);
        changed|=slider("Base height",f.base_height,0,10000);changed|=slider("Half height",f.half_height,0,10000);
        changed|=slider("Sun haze height",f.haze_height,-2000,10000);changed|=slider("Density scale",f.density,0,100);
    }
    if(ImGui::CollapsingHeader("Haze",ImGuiTreeNodeFlags_DefaultOpen)) {
        changed|=slider("Haze base distance",f.haze_base,0,5000);changed|=slider("Haze half distance",f.haze_half,0,5000);
        changed|=slider("Haze strength",f.haze_strength,-5,5);changed|=slider("Haze spread",f.haze_spread,0,std::max(1.f,f.haze_spread));
    }
    if(ImGui::CollapsingHeader("Lighting",ImGuiTreeNodeFlags_DefaultOpen)) {
        changed|=slider("Brightness",f.brightness,0,30);changed|=slider("Opacity",f.opacity,0,3);
        changed|=slider("PBR amount",f.pbr,0,1.5f);changed|=slider("Lit fog density",f.lit_density,0,1.5f);
    }
    ImGui::EndDisabled();if(changed)send(operation::change_fog,s.values);
    if(ImGui::CollapsingHeader("Fog presets",ImGuiTreeNodeFlags_DefaultOpen)) {
        file_controls("fog files",s.presets,operation::save_fog,operation::load_fog,"SAVE / LOAD PRESET",scale);
        if(ImGui::Button("Generate random fog",{-1,26*scale}))send(operation::random_fog);
    }
    ImGui::PopStyleVar(2);
}
void sun_section(snapshot& s,float scale) {
    ImGui::BeginDisabled(!s.sun_ready);auto& sun=s.values.sunlight;
    bool color_changed=color_control("Sun color",sun.tint,scale);ImGui::Spacing();title("SUN LIGHTING");
    bool changed=color_changed;
    changed|=slider("Brightness",sun.brightness,0,32000,"%.2f",true);changed|=slider("Bloom",sun.bloom,0,300);
    changed|=slider("Shadow aliasing",sun.shadows,0,30);ImGui::Spacing();title("DIRECTION");
    changed|=slider("Direction X",sun.x,-360,360,"%.2f deg");changed|=slider("Direction Y",sun.y,-360,360,"%.2f deg");
    if(ImGui::Button("Reset to film defaults",{-1,30*scale}))send(operation::reset_sun);
    ImGui::EndDisabled();if(changed)send(operation::change_sun,s.values,color_changed?1:0);
}
void lights_section(snapshot& s,float scale) {
    ImGui::BeginDisabled(!s.lights_ready);auto& lights=s.values.lights;
    selected_light=lights.empty()?-1:std::clamp(selected_light,0,int(lights.size())-1);
    char label[80];if(selected_light>=0)std::snprintf(label,sizeof(label),"Light %02d / %s",selected_light+1,lights[selected_light].type?"Spotlight":"Omnidirectional");else std::snprintf(label,sizeof(label),"No lights - add one at the camera");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("##selected light",label)) {
        for(int i=0;i<int(lights.size());++i) {ImGui::PushID(i);std::snprintf(label,sizeof(label),"Light %02d / %s",i+1,lights[i].type?"Spotlight":"Omnidirectional");if(ImGui::Selectable(label,i==selected_light))selected_light=i;ImGui::PopID();}ImGui::EndCombo();
    }
    if(ImGui::Button("Add light at camera"))send(operation::add_light);ImGui::SameLine();
    std::snprintf(label,sizeof(label),"%zu / %d",lights.size(),maximum_lights);hint(label);
    if(selected_light>=0) {
        auto& l=lights[selected_light];bool changed=color_control("Light color",l.tint,scale);
        ImGui::Spacing();title("LIGHT TYPE");changed|=radio("Omnidirectional",&l.type,0);ImGui::SameLine();changed|=radio("Spotlight",&l.type,1);
        changed|=slider("Intensity",l.intensity,0,700);changed|=slider("Range",l.range,0,7000,"%.0f units");
        float width=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*.5f;
        if(ImGui::Button("Go to light",{width,30*scale}))send(operation::goto_light,{},selected_light);ImGui::SameLine();
        if(ImGui::Button("Delete light",{width,30*scale}))send(operation::delete_light,{},selected_light);
        if(changed)send(operation::change_light,s.values,selected_light);
    }
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    file_controls("light files",s.lighting,operation::save_lights,operation::load_lights,"LIGHTING FILES",scale);ImGui::EndDisabled();
}
void misc_section(snapshot& s,const theater::snapshot& film) {
    ImGui::BeginDisabled(!s.misc_ready);auto& v=s.values;bool changed=false;
    title("VISIBILITY");changed|=checkbox("Remove gun",&v.remove_gun);ImGui::SameLine();changed|=checkbox("Draw 2D",&v.draw_2d);
    changed|=checkbox("Remove HUD",&v.remove_hud);ImGui::SameLine();ImGui::BeginDisabled(!s.blood_ready);changed|=checkbox("Remove blood overlay",&v.remove_blood);ImGui::EndDisabled();
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();changed|=checkbox("Enable scene overrides",&v.misc_enabled);
    ImGui::BeginDisabled(!v.misc_enabled);changed|=slider("Exposure",v.exposure,0,18);changed|=slider("Sky rotation",v.sky_rotation,0,360,"%.2f deg");changed|=checkbox("Sky transition",&v.sky_transition);
    ImGui::Spacing();title("GUN POSITION");changed|=slider("Gun X",v.gun.x,-10,10);changed|=slider("Gun Y",v.gun.y,-10,10);changed|=slider("Gun Z",v.gun.z,-10,10);ImGui::EndDisabled();ImGui::EndDisabled();
    if(changed)send(operation::change_misc,s.values);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();title("TIMESCALE");
    static float timescale=1;static bool initialized=false;if(!initialized) {timescale=film.speed;initialized=true;}
    ImGui::SetNextItemWidth(150*ImGui::GetFontSize()/20.f);ImGui::InputFloat("##timescale",&timescale,0,0,"%.3fx");ImGui::SameLine();
    if(ImGui::Button("Send"))enqueue({operation::set_timescale,{},-1,"",std::isfinite(timescale)?std::clamp(timescale,.01f,10.f):film.speed});
    ImGui::SameLine();hint("0.01x - 10x");
}
void configs_section(snapshot& s,float scale) {
    file_controls("configs",s.configs,operation::save_cfg,operation::load_cfg,"SAVE / LOAD CFG",scale);
    ImGui::Spacing();wrapped("CFG files store movement speed, FOV, timescale, fog, sun, lights, visibility, exposure, sky, and gun offsets. Save separate files for different looks.");
    ImGui::Spacing();hint("MVM/Theater/configs");ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button("Restore all",{170*scale,34*scale}))send(operation::restore);ImGui::SameLine();
    if(ImGui::Button("Reload",{170*scale,34*scale}))send(operation::reload);
    ImGui::Spacing();wrapped("Restore all returns this film to its captured defaults. Reload rescans game settings and refreshes files without discarding your current look.");
}
void settings_section(options settings) {
    title("INPUT SETTINGS");bool changed=false;
    changed|=slider("Roll per notch",settings.roll_step,.1f,15,"%.1f deg");changed|=slider("FOV per notch",settings.fov_step,.1f,10,"%.1f");
    changed|=slider("Seek step",settings.seek_seconds,.1f,30,"%.1f sec");ImGui::Spacing();
    changed|=checkbox("Show camera markers",&settings.show_markers);changed|=checkbox("Show keyboard hints",&settings.show_help);
    if(changed)set_options(settings);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();title("VIDEO RECORDING");
    auto recording=capture::get_status();auto capture_settings=capture::get_settings();bool updated=false;
    text("Frames per second");ImGui::SetNextItemWidth(-1);
    updated|=ImGui::SliderInt("##record_fps",&capture_settings.fps,1,240);
    const char* codecs[]={"ProRes 422 HQ (.mov)","FFV1 lossless (.mkv)","ProRes 4444 (.mov)"};
    text("Video format");ImGui::SetNextItemWidth(-1);
    updated|=ImGui::Combo("##record_codec",&capture_settings.codec,codecs,3);
    updated|=checkbox("Use BO3 fixed frame time when available",&capture_settings.fixed_step);
    if(updated)capture::set_settings(capture_settings);
    ImGui::BeginDisabled(recording.finalizing);
    if(ImGui::Button(recording.recording?"Stop recording":"Start recording",{-1,30*ImGui::GetFontSize()/20.f}))capture::toggle();
    ImGui::EndDisabled();
    wrapped(recording.message.c_str());
    if(recording.recording) {
        ImGui::Text("%llu frames captured",recording.frames);
        hint(recording.fixed_step?"BO3 fixed frame time active":"BO3 fixed frame time unavailable; output timing may vary");
    }
    if(!recording.file.empty())wrapped(recording.file.c_str());
    wrapped("Video only. ReShade finish-effects capture is used when the add-on hook is active; the theater overlay stays out of the video. Files save under MVM/Theater/recordings.");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();wrapped("Keep CFG files in MVM/Theater/configs, fog presets in MVM/Theater/fog-presets, and lighting in MVM/Theater/lighting.");
}
void binds_section(float scale) {
    title("THEATER KEY BINDS");
    wrapped("Click a bind, then press a key, mouse button, or turn the wheel. Modifiers work with all controls. Escape cancels. Conflicting binds swap places.");
    ImGui::Spacing();
    for(int n=0;n<int(binds::id::count);++n){
        auto action=binds::id(n);ImGui::PushID(n);
        float button_width=std::min(175.f*scale,ImGui::GetContentRegionAvail().x*.42f);
        const bool waiting=binds::listening_for()==action;
        std::string button=waiting?"Press a control...":binds::label(action);
        if(ImGui::Button(button.c_str(),{button_width,27*scale}))binds::listen(action);
        ImGui::SameLine();text(binds::title(action));ImGui::PopID();
    }
    ImGui::Spacing();
    if(ImGui::Button("Restore default binds"))binds::defaults();
    hint("Saved automatically in MVM/Theater/binds.json");
}
}
void select_section(int index) {section=std::clamp(index,0,7);}
void draw_menu(float scale) {
    auto s=current();auto film=current_snapshot();auto settings=current_options();auto screen=ImGui::GetIO().DisplaySize;
    transparent_menu=!settings.menu_background;
    float width=std::min(520.f*scale,screen.x-24*scale),height=screen.y-244*scale;
    ImGui::SetNextWindowPos({12*scale,12*scale});ImGui::SetNextWindowSize({width,height});
    ImGui::SetNextWindowBgAlpha(.94f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12*scale,10*scale});ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6*scale,6*scale});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{6*scale,3*scale});ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{6*scale,3*scale});
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,10*scale);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,3*scale);ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,3*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,transparent_menu?0.f:1.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,{0,0,0,0});
    auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar;
    if(transparent_menu)flags|=ImGuiWindowFlags_NoBackground;
    ImGui::Begin("T7MVM scene editor",nullptr,flags);ImGui::SetWindowFontScale(scale);
    text("T7MVM / SCENE EDITOR",{.24f,.74f,.87f,1});ImGui::SameLine(width-124*scale);
    std::string close_label="Close ["+binds::label(binds::id::menu)+"]";
    if(ImGui::SmallButton(close_label.c_str()))menu_open=false;
    bool no_background=!settings.menu_background;
    if(checkbox("No background",&no_background)) {settings.menu_background=!no_background;set_options(settings);}
    ImGui::SameLine();hint("Live scene adjustments");ImGui::Separator();
    // Two compact rows keep every section reachable at 720p.
    for(int i=0;i<8;++i) {
        ImGui::PushID(i);if(i%4)ImGui::SameLine(0,2*scale);
        float tab_width=ImGui::CalcTextSize(sections[i]).x+12*scale;auto pos=ImGui::GetCursorScreenPos();
        if(ImGui::Selectable(sections[i],section==i,0,{tab_width,26*scale}))section=i;
        if(transparent_menu) {shadow(pos,sections[i]);ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,ImGui::GetColorU32(ImGuiCol_Text),sections[i]);}
        ImGui::PopID();
    }
    ImGui::Separator();float footer=48*scale;
    ImGui::BeginChild("Settings content",{0,-footer},false,ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::SetWindowFontScale(1);
    switch(section) {case 0:sliders(film,settings,s);break;case 1:fog_section(s,scale);break;case 2:sun_section(s,scale);break;case 3:lights_section(s,scale);break;case 4:misc_section(s,film);break;case 5:configs_section(s,scale);break;case 6:settings_section(settings);break;case 7:binds_section(scale);break;}
    ImGui::EndChild();ImGui::Separator();
    // Keep long status messages inside the narrow panel; the full message is
    // available on hover so an imported path cannot grow the footer.
    std::string status=s.message;float available=ImGui::GetContentRegionAvail().x;
    while(status.size()>3 && ImGui::CalcTextSize(status.c_str()).x>available) {status.resize(status.size()-4);status+="...";}
    text(status.c_str());if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",s.message.c_str());
    hint("Ctrl + click a slider for precise input.");
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar(8);
}
}
