#include "preferences.hpp"
#include "vendor/json.hpp"
#include <fstream>
namespace theater::preferences {
options sanitize(options value) {
    auto clamp=[](float x,float fallback,float minimum,float maximum){return std::isfinite(x)?std::clamp(x,minimum,maximum):fallback;};
    value.move_speed=camera_speed(value.move_speed);
    value.fast_multiplier=clamp(value.fast_multiplier,2,1,100);
    value.slow_multiplier=clamp(value.slow_multiplier,.25f,.0001f,1);
    value.roll_step=clamp(value.roll_step,1,.1f,15);value.fov_step=clamp(value.fov_step,1,.1f,10);
    value.seek_seconds=clamp(value.seek_seconds,5,.1f,30);
    value.weapon_release_delay=clamp(value.weapon_release_delay,.65f,.05f,5);
    if(value.release_weapons)value.hold_weapons=true;
    if(value.hold_weapons)value.native_weapon_drops=false;
    return value;
}
options load(const std::filesystem::path& path) {
    options value;
    try {
        std::ifstream in(path);if(!in)return value;
        nlohmann::json data;in>>data;
        value.move_speed=data.value("moveSpeed",1.f);
        value.fast_multiplier=data.value("fastMultiplier",2.f);value.slow_multiplier=data.value("slowMultiplier",.25f);
        value.roll_step=data.value("rollStep",1.f);value.fov_step=data.value("fovStep",1.f);value.seek_seconds=data.value("seekSeconds",5.f);
        value.show_markers=data.value("showMarkers",true);value.show_help=data.value("showHelp",true);value.menu_background=data.value("menuBackground",true);
        value.responsive_camera=data.value("responsiveCamera",true);
        value.hold_weapons=data.value("holdWeapons",true);
        value.release_weapons=data.value("releaseWeapons",false);value.weapon_release_delay=data.value("weaponReleaseDelay",.65f);
        value.native_weapon_drops=data.value("nativeWeaponDrops",false);
        return sanitize(value);
    } catch(...) {return {};}
}
bool save(options value,const std::filesystem::path& path) {
    try {
        value=sanitize(value);std::filesystem::create_directories(path.parent_path());
        nlohmann::json data={{"version",1},{"moveSpeed",value.move_speed},{"fastMultiplier",value.fast_multiplier},
            {"slowMultiplier",value.slow_multiplier},{"rollStep",value.roll_step},{"fovStep",value.fov_step},
            {"seekSeconds",value.seek_seconds},{"showMarkers",value.show_markers},{"showHelp",value.show_help},
            {"menuBackground",value.menu_background},{"responsiveCamera",value.responsive_camera},{"holdWeapons",value.hold_weapons},{"releaseWeapons",value.release_weapons},
            {"weaponReleaseDelay",value.weapon_release_delay},{"nativeWeaponDrops",value.native_weapon_drops}};
        auto temporary=path;temporary+=".tmp";
        {std::ofstream out(temporary,std::ios::trunc);out<<data.dump(2)<<'\n';out.flush();if(!out)return false;}
        // ReplaceFile preserves the previous valid file if replacement fails.
        if(std::filesystem::exists(path))return ReplaceFileW(path.c_str(),temporary.c_str(),nullptr,0,nullptr,nullptr)!=FALSE;
        std::filesystem::rename(temporary,path);return true;
    } catch(...) {return false;}
}
}
