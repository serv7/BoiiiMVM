#include "animations.hpp"
#include "backend.hpp"
#include "gif_preview.hpp"
#include "bo2_gif_catalog.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>
namespace theater::animations {
void draw_weapon_controls(float scale) {
    auto settings=current_options();bool changed=false;
    if(ImGui::Checkbox("Hold weapon during death",&settings.hold_weapons)){
        if(settings.hold_weapons)settings.native_weapon_drops=false;else settings.release_weapons=false;changed=true;
    }
    if(ImGui::TreeNode("Weapon release options")){
    if(ImGui::Checkbox("Release held weapon with physics",&settings.release_weapons)){
        if(settings.release_weapons){settings.hold_weapons=true;settings.native_weapon_drops=false;}changed=true;
    }
    if(settings.release_weapons){ImGui::SetNextItemWidth(-1);changed|=ImGui::SliderFloat("##release_delay",&settings.weapon_release_delay,.05f,5,"Release after %.2f demo seconds");}
    if(ImGui::Checkbox("Native drops in new bot matches",&settings.native_weapon_drops)){
        if(settings.native_weapon_drops){settings.hold_weapons=false;settings.release_weapons=false;}changed=true;
    }
    ImGui::TreePop();}
    if(changed)set_options(settings);
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Rewind before the death after changing weapon options.");
}
void draw_menu(float scale,bool paused) {
    static char filter[128]{};
    static std::string target,source,preview;
    static bool manual_target=false,show_bo3=false;
    auto state=current();
    auto film=current_snapshot();
    if(preview.empty() && !state.active.empty())preview=state.active.front().name;
    if(preview.empty())preview=std::string(bo2_gif_catalog.front());
    ImGui::BeginDisabled(!paused || state.loading);
    if(ImGui::Button("Scan active deaths"))enqueue({operation::inspect_active,{},{},film.tick});
    ImGui::EndDisabled();ImGui::SameLine();ImGui::TextDisabled(state.loading?"One scan in progress...":"Scanning off");
    if(state.active_tick>=0)ImGui::TextDisabled("Snapshot at %.3f s / %zu clips",std::max(0,state.active_tick-film.start_tick)/1000.f,state.active.size());
    if(!paused)ImGui::TextWrapped("Pause during a death, then scan. Playback never triggers scanning.");
    if(ImGui::BeginTable("Death swap workspace",2,ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Choices",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Preview",ImGuiTableColumnFlags_WidthFixed,205*scale);
        ImGui::TableNextRow();ImGui::TableNextColumn();
        ImGui::TextColored({.24f,.74f,.87f,1},"TARGET / CAPTURED DEATHS");
        ImGui::BeginChild("Active death list",{0,82*scale},true);
        ImGui::SetWindowFontScale(scale);
        for(const auto& clip:state.active){
            std::string label=clip.name+"##"+std::to_string(clip.entity);
            if(ImGui::Selectable(label.c_str(),target==clip.name)){target=clip.name;preview=clip.name;manual_target=false;}
        }
        if(state.active.empty())ImGui::TextWrapped("Pause on a death and press Scan active deaths.");
        ImGui::EndChild();
        ImGui::TextWrapped("Target: %s",target.empty()?"Select a captured clip":target.c_str());
        ImGui::Spacing();ImGui::TextColored({.24f,.74f,.87f,1},"REPLACEMENT");
        if(ImGui::Button(show_bo3?"Show BO2 / GIF animations":"Show BO3 animations"))show_bo3=!show_bo3;
        ImGui::SameLine();ImGui::TextDisabled(show_bo3?"BO3":"BO2 / GIF");
        ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##death_filter","Search death animations",filter,sizeof(filter));
        ImGui::Checkbox("Pick target here",&manual_target);
        const auto& catalog=state.catalog?*state.catalog:state.entries;
        std::string search=filter;for(auto& c:search)c=char(std::tolower(static_cast<unsigned char>(c)));
        std::vector<size_t> filtered;filtered.reserve(catalog.size());
        for(size_t i=0;i<catalog.size();++i){
            const auto& name=catalog[i].name;
            bool gif=std::binary_search(bo2_gif_catalog.begin(),bo2_gif_catalog.end(),std::string_view(name));
            if((show_bo3 || gif) && (search.empty() || name.find(search)!=std::string::npos))filtered.push_back(i);
        }
        float list_height=std::clamp(ImGui::GetContentRegionAvail().y-170*scale,75*scale,220*scale);
        ImGui::BeginChild("Death clip list",{0,list_height},true);
        ImGui::SetWindowFontScale(scale);
        ImGuiListClipper clipper;clipper.Begin(int(filtered.size()));
        while(clipper.Step())for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row){
            const auto& clip=catalog[filtered[row]];
            if(ImGui::Selectable(clip.name.c_str(),(manual_target?target:source)==clip.name)){(manual_target?target:source)=clip.name;preview=clip.name;}
        }
        if(filtered.empty())ImGui::TextWrapped("No matching death animations.");
        ImGui::EndChild();
        ImGui::TextWrapped("Replacement: %s",source.empty()?"Choose a clip":source.c_str());
        ImGui::TableNextColumn();
        ImGui::TextColored({.24f,.74f,.87f,1},"PREVIEW");
        if(auto image=gif_preview::frame(preview))ImGui::Image(image,{190*scale,190*scale});
        else {ImGui::Dummy({190*scale,150*scale});ImGui::TextWrapped(gif_preview::loading()?"Loading GIF...":"No GIF for this BO3 clip.");}
        ImGui::TextDisabled("BO2 reference GIF");ImGui::TextWrapped("%s",preview.c_str());
        ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
        auto settings=current_options();
        if(ImGui::Checkbox("Hold gun in demo",&settings.hold_weapons)){
            if(settings.hold_weapons)settings.native_weapon_drops=false;else settings.release_weapons=false;
            set_options(settings);
        }
        ImGui::TextWrapped("Playback only. Weapon release options are in Misc.");
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::BeginDisabled(!paused || !state.ready || target.empty() || source.empty() || target==source);
    if(ImGui::Button("Replace",{0,28*scale}))enqueue({operation::replace,target,source});ImGui::EndDisabled();ImGui::SameLine();
    ImGui::BeginDisabled(!paused || !state.ready);
    if(ImGui::Button("Restore target",{0,28*scale}))enqueue({operation::restore,target});ImGui::SameLine();
    if(ImGui::Button("Restore all",{0,28*scale}))enqueue({operation::restore_all});ImGui::EndDisabled();
    ImGui::TextWrapped("%s",state.message.c_str());
    ImGui::TextWrapped("Rewind before the death after a swap. Scan again to update captured deaths.");
}
}
