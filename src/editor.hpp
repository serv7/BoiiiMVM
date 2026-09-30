#pragma once
#include "theater_logic.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace theater::editor {
constexpr int maximum_lights=25;
struct color {float r=1,g=1,b=1;};
inline bool has_sun_color(color value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
        std::max({value.r,value.g,value.b})>=.001f;
}
inline color visible_sun_color(color value) {return has_sun_color(value)?value:color{1.f,.94f,.82f};}
struct fog {
    float base_distance=200,half_distance=10000,base_height=3000,half_height=3000,haze_height=3000;
    std::array<unsigned char,40> padding{};
    color tint;
    float density=10;
    color haze_tint;
    float haze_base=200,haze_half=200,haze_strength=.2f,haze_spread=.2f;
    float brightness=5,opacity=1,pbr=1,lit_density=1,sky_size=65000;
};
struct sun {float x=100,y=100;color tint;float brightness=100,bloom=100,shadows=30,unused=0;};
struct light {
    int type=0;float intensity=10;color tint;float range=1000,intensity_scale=50;
    vec3 position;float look_x=0,look_y=0,look_z=0,u1=0,u2=0,u3=0;
};
static_assert(sizeof(fog)==124 && offsetof(fog,tint)==60 && offsetof(fog,haze_tint)==76);
static_assert(sizeof(sun)==36 && sizeof(light)==64 && offsetof(light,position)==28);
struct scene {
    fog mist;sun sunlight;
    bool fog_enabled=false,sync_colors=false,sync_sun=false,misc_enabled=false;
    bool remove_gun=false,draw_2d=true,remove_hud=false,remove_blood=false,sky_transition=false;
    float exposure=10,sky_rotation=0;vec3 gun;
    std::vector<light> lights;
};
struct config {scene values;float move_speed=1,fov=65,timescale=1,roll_step=1,fov_step=1,seek_seconds=5;bool markers=true,help=true,has_lights=true,menu_background=true;};
struct snapshot {
    scene values;bool ready=false,fog_ready=false,sun_ready=false,misc_ready=false,lights_ready=false,blood_ready=false;
    bool useful_dvars=false,useful_dvars_ready=false;
    std::string message="Scene editor ready",map;
    std::vector<std::string> configs,presets,lighting;
};
enum class operation {change_fog,change_sun,change_misc,change_light,reset_sun,restore,reload,
    goto_light,add_light,delete_light,save_cfg,load_cfg,save_fog,load_fog,random_fog,save_lights,load_lights,refresh,set_timescale,toggle_useful_dvars};
struct request {operation type;scene values;int index=-1;std::string name;float value=1;};
bool initialize(uintptr_t game);
void shutdown();
void begin(uintptr_t session);
void end();
void update();
snapshot current();
void enqueue(request value);
void draw_menu(float scale);
void select_section(int index);
// Called by the native backend with its session lock held.
void apply_controls(const config& value);
config capture_controls(scene value);
void goto_position(vec3 position);
void set_capture_frame_rate(int fps);
bool capture_frame_lock_active();
std::string serialize(const config& value);
config deserialize(const std::string& text);
fog deserialize_fog(const std::string& text);
std::string serialize_fog(const fog& value);
std::vector<light> deserialize_lights(const std::string& text);
std::string serialize_lights(const std::vector<light>& value);
uint32_t hash_name(const char* name);
uint32_t encode_float(float value,uint32_t peb);
uint32_t encode_int(int32_t value,uint32_t peb);
uint32_t encode_bool(bool value,uint32_t peb);
}
