#include "editor.hpp"
#include "vendor/json.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace theater::editor {
using json=nlohmann::json;
namespace {
float number(const json& j,const char* name,float fallback,float minimum=-1000000,float maximum=1000000) {
    float value=j.value(name,fallback);
    if(!std::isfinite(value) || value<minimum || value>maximum)throw std::runtime_error(std::string("Invalid value: ")+name);
    return value;
}
json rgb(color c) {return {{"r",c.r},{"g",c.g},{"b",c.b}};}
color rgb(const json& j) {return {number(j,"r",1,0,10000),number(j,"g",1,0,10000),number(j,"b",1,0,10000)};}
json vector(vec3 v) {return {{"X",v.x},{"Y",v.y},{"Z",v.z}};}
vec3 vector(const json& j) {return {number(j,"X",0),number(j,"Y",0),number(j,"Z",0)};}
json fog_json(const fog& f) {
    return {{"bDist",f.base_distance},{"hDist",f.half_distance},{"bHeight",f.base_height},{"hHeight",f.half_height},
        {"skyHalfHeightOffset",f.haze_height},{"fCol",rgb(f.tint)},{"fDensity",f.density},{"shCol",rgb(f.haze_tint)},
        {"shbDist",f.haze_base},{"shfDist",f.haze_half},{"shStrength",f.haze_strength},{"shSpread",f.haze_spread},
        {"Brightness",f.brightness},{"Opacity",f.opacity},{"pbrAmount",f.pbr},{"LitFogDensity",f.lit_density},{"WorldFogSkySize",f.sky_size}};
}
fog fog_json(const json& j) {
    if(!j.is_object() || !j.contains("fCol") || !j.contains("hDist"))throw std::runtime_error("Invalid fog preset");
    fog f;
    f.base_distance=number(j,"bDist",200,-100000,100000);f.half_distance=number(j,"hDist",10000,0,1000000);
    f.base_height=number(j,"bHeight",3000,-100000,100000);f.half_height=number(j,"hHeight",3000,0,1000000);
    f.haze_height=number(j,"skyHalfHeightOffset",3000,-100000,100000);
    f.tint=rgb(j.at("fCol"));f.density=number(j,"fDensity",10,0,10000);
    f.haze_tint=rgb(j.at("shCol"));f.haze_base=number(j,"shbDist",200,-100000,100000);
    f.haze_half=number(j,"shfDist",200,0,1000000);f.haze_strength=number(j,"shStrength",.2f,-100,100);
    f.haze_spread=number(j,"shSpread",.2f,0,100);f.brightness=number(j,"Brightness",5,0,10000);
    f.opacity=number(j,"Opacity",1,0,100);f.pbr=number(j,"pbrAmount",1,0,100);
    f.lit_density=number(j,"LitFogDensity",1,0,100);f.sky_size=number(j,"WorldFogSkySize",65000,0,1000000);
    return f;
}
json sun_json(const sun& s) {return {{"X",s.x},{"Y",s.y},{"Color",rgb(s.tint)},{"Brightness",s.brightness},{"bloom",s.bloom},{"shadows",s.shadows},{"idk",s.unused}};}
sun sun_json(const json& j) {sun s;s.x=number(j,"X",100,-360,360);s.y=number(j,"Y",100,-360,360);
    s.tint=rgb(j.at("Color"));s.brightness=number(j,"Brightness",100,0,32000);s.bloom=number(j,"bloom",100,0,1000);
    s.shadows=number(j,"shadows",30,0,100);s.unused=number(j,"idk",0);return s;}
json lights_json(const std::vector<light>& lights) {
    json items=json::array();
    for(auto& l:lights)items.push_back({{"Type",l.type},{"Intensity",l.intensity},{"Color",rgb(l.tint)},{"Range",l.range},
        {"IntensityScale",l.intensity_scale},{"position",vector(l.position)},{"XlookAt",l.look_x},{"YlookAt",l.look_y},
        {"ZlookAt",l.look_z},{"U1",l.u1},{"U2",l.u2},{"U3",l.u3}});
    return {{"NumberofLights",lights.size()},{"lights",items}};
}
std::vector<light> lights_json(const json& j) {
    int count=j.at("NumberofLights").get<int>();const auto& items=j.at("lights");
    if(count<0 || count>maximum_lights || !items.is_array() || items.size()<size_t(count) || items.size()>50)throw std::runtime_error("Lighting file must contain at most 25 active lights");
    std::vector<light> result;
    for(int i=0;i<count;++i) {
        auto& a=items[i];light l;l.type=a.at("Type").get<int>();if(l.type<0 || l.type>1)throw std::runtime_error("Invalid light type");
        l.intensity=number(a,"Intensity",10,0,100000);l.tint=rgb(a.at("Color"));l.range=number(a,"Range",1000,0,1000000);
        l.intensity_scale=number(a,"IntensityScale",1,0,100000);l.position=vector(a.at("position"));
        l.look_x=number(a,"XlookAt",0);l.look_y=number(a,"YlookAt",0);l.look_z=number(a,"ZlookAt",0);
        l.u1=number(a,"U1",0);l.u2=number(a,"U2",0);l.u3=number(a,"U3",0);result.push_back(l);
    }return result;
}
json parse(const std::string& text) {if(text.size()>1024*1024)throw std::runtime_error("File exceeds 1 MB");return json::parse(text);}
}
std::string serialize_fog(const fog& f) {return fog_json(f).dump(2);}
fog deserialize_fog(const std::string& text) {return fog_json(parse(text));}
std::string serialize_lights(const std::vector<light>& lights) {return lights_json(lights).dump(2);}
std::vector<light> deserialize_lights(const std::string& text) {return lights_json(parse(text));}
std::string serialize(const config& c) {
    auto& s=c.values;
    json j={{"format","T7MVM_SCENE"},{"version",1},{"fog",fog_json(s.mist)},{"sun",sun_json(s.sunlight)},
        {"fogEnabled",s.fog_enabled},{"syncColors",s.sync_colors},{"syncSun",s.sync_sun},{"enableMiscSettings",s.misc_enabled},
        {"cg_drawGun",!s.remove_gun},{"draw2d",s.draw_2d},{"removeHud",s.remove_hud},{"removeBlood",s.remove_blood},
        {"skyRotation",s.sky_rotation},{"skyTransition",s.sky_transition?1:0},{"exposure",s.exposure},{"GunPosition",vector(s.gun)},
        {"lighting",lights_json(s.lights)},{"moveSpeed",c.move_speed},{"fov",c.fov},{"timescale",c.timescale},
        {"rollStep",c.roll_step},{"fovStep",c.fov_step},{"seekSeconds",c.seek_seconds},{"showMarkers",c.markers},{"showHelp",c.help},{"menuBackground",c.menu_background}};
    return j.dump(2);
}
config deserialize(const std::string& text) {
    auto j=parse(text);config c;auto& s=c.values;
    if(!j.is_object() || !j.contains("sun") || !j.contains("fov"))throw std::runtime_error("Invalid CFG file");
    if(j.contains("format") && (j.at("format")!="T7MVM_SCENE" || j.at("version")!=1))throw std::runtime_error("Unsupported CFG version");
    s.mist=fog_json(j.at("fog"));s.sunlight=sun_json(j.at("sun"));
    s.fog_enabled=j.value("fogEnabled",!j.contains("format"));s.sync_colors=j.value("syncColors",false);s.sync_sun=j.value("syncSun",false);
    s.misc_enabled=j.value("enableMiscSettings",false);s.remove_gun=!j.value("cg_drawGun",true);s.draw_2d=j.value("draw2d",true);
    s.remove_hud=j.value("removeHud",false);s.remove_blood=j.value("removeBlood",false);
    s.sky_rotation=number(j,"skyRotation",0,0,360);s.sky_transition=number(j,"skyTransition",0,0,1)>0;
    s.exposure=number(j,"exposure",10,0,18);s.gun=vector(j.value("GunPosition",json::object()));
    if(std::abs(s.gun.x)>10 || std::abs(s.gun.y)>10 || std::abs(s.gun.z)>10)throw std::runtime_error("Gun offsets must be between -10 and 10");
    c.has_lights=j.contains("lighting");if(c.has_lights)s.lights=lights_json(j.at("lighting"));
    c.move_speed=number(j,"moveSpeed",1,.0001f,10);c.fov=number(j,"fov",65,.1f,75);c.timescale=number(j,"timescale",1,.01f,10);
    c.roll_step=number(j,"rollStep",1,.1f,15);c.fov_step=number(j,"fovStep",1,.1f,10);c.seek_seconds=number(j,"seekSeconds",5,.1f,30);
    c.markers=j.value("showMarkers",true);c.help=j.value("showHelp",true);c.menu_background=j.value("menuBackground",true);return c;
}
uint32_t hash_name(const char* name) {uint32_t result=0x4b9ace2f;do {unsigned char c=*name++;if(c>='A' && c<='Z')c+=32;result=16777619u*(result^c);if(!c)break;}while(true);return result;}
uint32_t encode_float(float value,uint32_t peb) {
    uint32_t result=std::bit_cast<uint32_t>(value),s=uint16_t(result);
    result=std::rotl(result,16);result^=uint16_t((peb^0xae44u)+s*450u);s=uint16_t(result);
    s=uint16_t((peb^0x8065u)-s*0x760bu);result=std::rotl(result,16);result^=s;s=uint16_t(result);
    result=std::rotl(result,16);s=uint16_t((peb^0x252u)+s*0x78f8u);result^=s;s=uint16_t(result);
    s=uint16_t((peb^0x9ffau)+s*0x2c23u);return std::rotl(s^std::rotl(result,16),16);
}
uint32_t encode_bool(bool value,uint32_t peb) {
    uint32_t x=value?1:0,z=uint16_t((peb^0xc8c0u)-x*0x56ddu);x=std::rotr(x,16);x^=z;
    x=uint16_t((peb^0xc3ecu)-x*0xc67u)^std::rotr(x,16);z=uint16_t(x);
    z=uint16_t((peb^0xbc84u)-z*0x1c72u);uint32_t t=z^std::rotl(x,16);
    z=uint16_t((peb^0xc8adu)+t*0x5c92u);x=std::rotl(t,16);return std::rotr(x^z,16);
}
}

namespace theater::editor {
uint32_t encode_int(int32_t value,uint32_t peb) {
    uint32_t result=std::bit_cast<uint32_t>(value);
    uint32_t y=uint16_t((peb^0x7c35u)-result*0x565bu);
    result=std::rotl(result,16);
    uint32_t temp=result^y;
    y=uint16_t((peb^0x6bfau)+temp*0x54f1u);
    result=std::rotr(temp,16)^y;
    y=uint16_t(result);
    y=uint16_t((peb^0xffff9674u)-y*0x5534u);
    result=std::rotl(result,16)^y;
    y=uint16_t(result);
    y=uint16_t((peb^0x6a32u)+y*0x1757u);
    return std::rotl(std::rotr(result,16)^y,16);
}
}
