#include "editor.hpp"
#include "backend.hpp"
#include <Windows.h>
#include <MH/MinHook.h>
#include <mutex>
#include <deque>
#include <map>
#include <filesystem>
#include <fstream>
#include <random>
#include <cstring>
#include <intrin.h>
#include <atomic>
namespace theater::editor {
namespace {
uintptr_t base=0,session=0;
std::mutex mutex,fog_mutex;
snapshot state;
scene baseline;
config original_controls;
std::deque<request> pending;
bool sun_owned=false,lights_owned=false,misc_owned=false,blood_owned=false,fog_hook=false,active=false;
bool sun_color_captured=false;
fog observed_fog,override_fog;
bool observed=false,fog_enabled=false;
uintptr_t blood_cg=0;std::array<unsigned char,3> blood_state{};
std::map<std::string,uintptr_t> dvars;
std::map<std::string,uint32_t> dvar_hashes;
struct saved_dvar {std::array<unsigned char,32> current;unsigned char modified;};
std::map<uintptr_t,saved_dvar> saved;
std::atomic<int> capture_fps=0;
std::atomic<bool> fixed_step_active=false;
int applied_capture_fps=0;
std::vector<light> original_lights;
void* (*fog_original)(void*,const void*)=nullptr;
constexpr uintptr_t fog_copy=0x1ca82f0,sun_address=80930164,blood_address=3798102;
constexpr unsigned char fog_code[]={0x8b,0x02,0x89,0x01,0x8b,0x42,0x04,0x89,0x41,0x04};
constexpr unsigned char blood_code[]={136,132,26,160,100,19,0,68,136,188,26,162,100,19,0,68,136,164,26,161,100,19,0};
template<class T> T& at(uintptr_t p,size_t offset=0) {return *reinterpret_cast<T*>(p+offset);}
bool accessible(uintptr_t p,size_t n,bool write=false) {
    MEMORY_BASIC_INFORMATION m{};
    if(!p || !VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) || m.State!=MEM_COMMIT ||
        (m.Protect&(PAGE_NOACCESS|PAGE_GUARD)) || n>m.RegionSize || p+n>uintptr_t(m.BaseAddress)+m.RegionSize)return false;
    return !write || (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
void note(std::string message) {state.message=std::move(message);log_message("Editor: "+state.message);}
void* copy_fog(void* destination,const void* source) {
    // Native structure: 20-byte header, followed by 124 bytes of fog. Keep the
    // header and opaque padding from the renderer, including per-map flags.
    std::lock_guard lock(fog_mutex);
    if(active) {
        std::memcpy(&observed_fog,static_cast<const unsigned char*>(source)+20,sizeof(fog));observed=true;
        if(fog_enabled) {
            std::array<unsigned char,144> buffer;std::memcpy(buffer.data(),source,buffer.size());
            fog value=override_fog;value.padding=observed_fog.padding;
            std::memcpy(buffer.data()+20,&value,sizeof(value));
            return fog_original(destination,buffer.data());
        }
    }
    return fog_original(destination,source);
}
bool patch_blood(bool enabled) {
    if(enabled==blood_owned)return true;
    auto p=reinterpret_cast<void*>(base+blood_address);DWORD old;
    if(!accessible(uintptr_t(p),sizeof(blood_code)) || !VirtualProtect(p,sizeof(blood_code),PAGE_EXECUTE_READWRITE,&old))return false;
    if(enabled)std::memset(p,0x90,sizeof(blood_code));else std::memcpy(p,blood_code,sizeof(blood_code));
    VirtualProtect(p,sizeof(blood_code),old,&old);FlushInstructionCache(GetCurrentProcess(),p,sizeof(blood_code));blood_owned=enabled;
    if(!enabled && blood_cg) {if(accessible(blood_cg+0x1364a0,3,true))std::memcpy(reinterpret_cast<void*>(blood_cg+0x1364a0),blood_state.data(),3);blood_cg=0;}
    return true;
}
void clear_blood() {
    if(!blood_owned || !accessible(base+397468560,8))return;
    auto cg=at<uintptr_t>(base+397468560);if(!accessible(cg+0x1364a0,3,true))return;
    if(!blood_cg) {blood_cg=cg;std::memcpy(blood_state.data(),reinterpret_cast<void*>(cg+0x1364a0),3);}
    if(cg==blood_cg)std::memset(reinterpret_cast<void*>(cg+0x1364a0),0,3);
}
uintptr_t dvar(const char* name) {auto it=dvars.find(name);return it==dvars.end()?0:it->second;}
bool write_dvar(const char* name,float value,bool boolean=false) {
    auto p=dvar(name);if(!accessible(p,160,true) || at<uint32_t>(p)!=dvar_hashes[name] || at<uint32_t>(p,28)!=(boolean?1u:2u))throw std::runtime_error(std::string("Setting is unavailable: ")+name);
    if(!saved.contains(p)) {saved_dvar original;std::memcpy(original.current.data(),reinterpret_cast<void*>(p+40),32);original.modified=at<unsigned char>(p,32);saved[p]=original;}
    uint32_t peb=uint32_t(__readgsqword(0x60));
    if(boolean)at<bool>(p,40)=value!=0;else at<float>(p,40)=value;
    at<uint64_t>(p,64)=boolean?encode_bool(value!=0,peb):encode_float(value,peb);
    at<unsigned char>(p,32)=1;return true;
}
void write_useful_dvar(const char* name,int32_t value) {
    auto p=dvar(name);uint32_t type=accessible(p,160,true)?at<uint32_t>(p,28):0;
    if(!p || at<uint32_t>(p)!=dvar_hashes[name])throw std::runtime_error(std::string("Setting is unavailable: ")+name);
    if(type==1 || type==2) {write_dvar(name,float(value),type==1);return;}
    if(type!=6 && type!=7)throw std::runtime_error(std::string("Unsupported setting type: ")+name);
    if(!saved.contains(p)) {saved_dvar original;std::memcpy(original.current.data(),reinterpret_cast<void*>(p+40),32);original.modified=at<unsigned char>(p,32);saved[p]=original;}
    at<int32_t>(p,40)=value;
    at<uint64_t>(p,64)=encode_int(value,uint32_t(__readgsqword(0x60)));
    at<unsigned char>(p,32)=1;
}
void restore_dvar(const char* name) {
    auto p=dvar(name);auto it=saved.find(p);if(it==saved.end())return;
    if(accessible(p,160,true)) {std::memcpy(reinterpret_cast<void*>(p+40),it->second.current.data(),32);at<unsigned char>(p,32)=it->second.modified;}
    saved.erase(it);
}
void find_dvars() {
    dvars.clear();dvar_hashes.clear();state.useful_dvars_ready=false;auto pool=base+397173280;
    if(!accessible(base+397173196,4))return;
    int count=at<int>(base+397173196);if(count<0 || count>20000 || !accessible(pool,size_t(count)*160))return;
    const char* names[]={"cg_gun_x","cg_gun_y","cg_gun_z","r_exposureValue","r_exposureTweak","r_skyRotation","r_skyTransition","cg_drawGun","cg_draw2d","cg_drawCrosshair"};
    for(auto name:names)for(int i=0;i<count;++i) {
        auto p=pool+i*160;if(at<uint32_t>(p)==hash_name(name)) {
            uint32_t type=at<uint32_t>(p,28);
            // Follow SESSIONMODE_BASE_DVAR through its native mode pool. MP=1.
            if(type==15) {p=at<uintptr_t>(p,48);type=accessible(p,160)?at<uint32_t>(p,28):0;}
            const bool boolean=std::string(name)=="r_exposureTweak" || std::string(name).starts_with("cg_draw");
            if(accessible(p,160,true) && type==(boolean?1u:2u)) {dvars[name]=p;dvar_hashes[name]=at<uint32_t>(p);}break;
        }
    }
    constexpr const char* useful[]={"r_DedicatedPlayerShadowCull","r_DedicatedPlayerSunShadowResolution","demo_dollycamHighlightThreshholdDistance"};
    state.useful_dvars_ready=true;
    for(auto name:useful) {
        bool found=false;
        for(int i=0;i<count;++i) {
            auto p=pool+i*160;if(at<uint32_t>(p)!=hash_name(name))continue;
            uint32_t type=at<uint32_t>(p,28);
            if(type==15) {p=at<uintptr_t>(p,48);type=accessible(p,160)?at<uint32_t>(p,28):0;}
            if(accessible(p,160,true) && (type==1 || type==2 || type==6 || type==7)) {dvars[name]=p;dvar_hashes[name]=at<uint32_t>(p);found=true;}
            break;
        }
        if(!found){
            state.useful_dvars_ready=false;
            log_message(std::string("Useful dvar unavailable in this film: ")+name);
        }
    }
    for(int i=0;i<count;++i) {
        auto p=pool+i*160;
        if(at<uint32_t>(p)!=hash_name("com_fixedtime_float"))continue;
        uint32_t type=at<uint32_t>(p,28);
        if(type==15){p=at<uintptr_t>(p,48);type=accessible(p,160)?at<uint32_t>(p,28):0;}
        if(accessible(p,160,true) && type==2){dvars["com_fixedtime_float"]=p;dvar_hashes["com_fixedtime_float"]=at<uint32_t>(p);}
        break;
    }
    state.misc_ready=std::all_of(std::begin(names),std::end(names),[](const char* name){return dvars.contains(name);});
    if(!state.misc_ready)note("Some scene dvars are unavailable; Reload rescans them");
}
float read_float(const char* name,float fallback) {auto p=dvar(name);float value=p?at<float>(p,40):fallback;return std::isfinite(value)?value:fallback;}
bool read_bool(const char* name,bool fallback) {auto p=dvar(name);return p?at<bool>(p,40):fallback;}
std::vector<light> read_lights() {
    constexpr size_t offset=9979404,count_offset=9981004;
    if(!accessible(session+count_offset,4))return {};
    int count=at<int>(session,count_offset);
    state.lights_ready=count>=0 && count<=maximum_lights && accessible(session+offset,maximum_lights*sizeof(light),true);
    if(!state.lights_ready)return {};
    auto p=reinterpret_cast<light*>(session+offset);return {p,p+count};
}
void write_lights(const std::vector<light>& values) {
    if(!state.lights_ready || values.size()>maximum_lights)throw std::runtime_error("Native light storage unavailable");
    if(!lights_owned) {original_lights=read_lights();lights_owned=true;}
    if(!values.empty())std::memcpy(reinterpret_cast<void*>(session+9979404),values.data(),values.size()*sizeof(light));
    at<int>(session,9981004)=int(values.size());state.values.lights=values;
}
void apply_fog() {
    if(state.values.sync_colors)state.values.mist.haze_tint=state.values.mist.tint;
    if(state.values.sync_sun && state.sun_ready) {state.values.sunlight.tint=state.values.mist.tint;sun_owned=true;}
    std::lock_guard lock(fog_mutex);override_fog=state.values.mist;fog_enabled=state.values.fog_enabled && state.fog_ready;
}
void apply_misc() {
    if(!state.misc_ready)throw std::runtime_error("Scene dvars unavailable; use Reload after the film has loaded");
    if(state.values.misc_enabled) {
        write_dvar("r_exposureTweak",1,true);write_dvar("r_exposureValue",state.values.exposure);
        write_dvar("r_skyRotation",state.values.sky_rotation);write_dvar("r_skyTransition",state.values.sky_transition?1.f:0.f);
        write_dvar("cg_gun_x",state.values.gun.x);write_dvar("cg_gun_y",state.values.gun.y);write_dvar("cg_gun_z",state.values.gun.z);misc_owned=true;
    } else {
        for(auto name:{"r_exposureTweak","r_exposureValue","r_skyRotation","r_skyTransition","cg_gun_x","cg_gun_y","cg_gun_z"})restore_dvar(name);
        misc_owned=false;
    }
    write_dvar("cg_drawGun",state.values.remove_gun?0.f:1.f,true);write_dvar("cg_draw2d",state.values.draw_2d?1.f:0.f,true);
    at<bool>(session,0x68abbd)=state.values.remove_hud;
    if(state.values.remove_hud)write_dvar("cg_drawCrosshair",0,true);else restore_dvar("cg_drawCrosshair");
    if(state.blood_ready && !patch_blood(state.values.remove_blood))throw std::runtime_error("Could not update blood overlay");
}
std::filesystem::path folder(operation op) {
    if(op==operation::save_fog || op==operation::load_fog)return "MVM/Theater/fog-presets";
    if(op==operation::save_lights || op==operation::load_lights)return "MVM/Theater/lighting";
    return "MVM/Theater/configs";
}
std::filesystem::path filename(const request& r) {
    std::filesystem::path leaf=r.name;
    if(leaf.empty() || leaf.has_parent_path() || r.name.find_first_of("<>:\"/\\|?*")!=std::string::npos || r.name=="." || r.name=="..")throw std::runtime_error("Enter a filename, without a folder path");
    if(!leaf.has_extension())leaf+=r.type==operation::save_fog?".zfog":r.type==operation::save_lights?".bo3light":".cfg";
    return folder(r.type)/leaf;
}
void refresh_files() {
    auto list=[](const char* dir,const std::vector<std::string>& extensions) {
        std::filesystem::create_directories(dir);std::vector<std::string> result;
        for(auto& item:std::filesystem::directory_iterator(dir))if(item.is_regular_file() && std::find(extensions.begin(),extensions.end(),item.path().extension().string())!=extensions.end())result.push_back(item.path().filename().string());
        std::sort(result.begin(),result.end());return result;
    };
    state.configs=list("MVM/Theater/configs",{".cfg",".bo3"});state.presets=list("MVM/Theater/fog-presets",{".zfog"});state.lighting=list("MVM/Theater/lighting",{".bo3light"});
}
std::string read_file(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>1024*1024)throw std::runtime_error("File exceeds 1 MB");
    std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Could not open file");return {std::istreambuf_iterator<char>(in),{}};
}
void save_file(const std::filesystem::path& path,const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());auto temp=path;temp+=".tmp";
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<contents;out.flush();if(!out)throw std::runtime_error("Could not save file");out.close();
    if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not replace file");
}
void restore(bool controls) {
    capture_fps=0;fixed_step_active=false;applied_capture_fps=0;
    {std::lock_guard lock(fog_mutex);fog_enabled=false;}
    if(sun_owned && accessible(base+sun_address,sizeof(sun),true))at<sun>(base+sun_address)=baseline.sunlight;
    if(lights_owned && state.lights_ready && accessible(session+9979404,maximum_lights*sizeof(light),true)) {
        if(!original_lights.empty())std::memcpy(reinterpret_cast<void*>(session+9979404),original_lights.data(),original_lights.size()*sizeof(light));
        at<int>(session,9981004)=int(original_lights.size());
    }
    for(auto& [p,data]:saved)if(accessible(p,160,true)) {std::memcpy(reinterpret_cast<void*>(p+40),data.current.data(),32);at<unsigned char>(p,32)=data.modified;}
    saved.clear();state.useful_dvars=false;if(blood_owned)patch_blood(false);
    if(session && accessible(session+0x68abbd,1,true))at<bool>(session,0x68abbd)=baseline.remove_hud;
    state.values=baseline;sun_owned=false;lights_owned=false;misc_owned=false;
    if(controls)apply_controls(original_controls);
}
void execute(const request& r) {
    switch(r.type) {
    case operation::change_fog:state.values.mist=r.values.mist;state.values.fog_enabled=r.values.fog_enabled;state.values.sync_colors=r.values.sync_colors;state.values.sync_sun=r.values.sync_sun;apply_fog();break;
    case operation::change_sun:
        if(!state.sun_ready)throw std::runtime_error("Sun controls unavailable");
        state.values.sunlight=r.values.sunlight;
        // A film may expose its sun block before the map fills its color.
        // Slider edits should never commit that transient black tint. The
        // picker can still deliberately set black (index 1).
        if(r.index!=1)state.values.sunlight.tint=visible_sun_color(state.values.sunlight.tint);
        sun_owned=true;break;
    case operation::change_misc: {
        scene previous=state.values;
        state.values.misc_enabled=r.values.misc_enabled;state.values.remove_gun=r.values.remove_gun;state.values.draw_2d=r.values.draw_2d;
        state.values.remove_hud=r.values.remove_hud;state.values.remove_blood=r.values.remove_blood;state.values.exposure=r.values.exposure;
        state.values.sky_rotation=r.values.sky_rotation;state.values.sky_transition=r.values.sky_transition;state.values.gun=r.values.gun;
        try {apply_misc();}catch(...) {state.values=previous;throw;}break;
    }
    case operation::change_light: {
        auto values=read_lights();if(r.index<0 || r.index>=int(values.size()) || r.index>=int(r.values.lights.size()))throw std::runtime_error("Select a light first");
        values[r.index]=r.values.lights[r.index];write_lights(values);break;
    }
    case operation::reset_sun:state.values.sunlight=baseline.sunlight;sun_owned=true;note("Sun restored to the film's original values");break;
    case operation::restore:restore(true);note("All scene overrides restored");break;
    case operation::reload:
        if(state.useful_dvars) {for(auto name:{"r_DedicatedPlayerShadowCull","r_DedicatedPlayerSunShadowResolution","demo_dollycamHighlightThreshholdDistance"})restore_dvar(name);state.useful_dvars=false;}
        find_dvars();refresh_files();note("Reloaded game addresses and file lists");break;
    case operation::refresh:refresh_files();note("File lists refreshed");break;
    case operation::goto_light: {auto lights=read_lights();if(r.index<0 || r.index>=int(lights.size()))throw std::runtime_error("Select a light first");goto_position(lights[r.index].position);note("Moved camera to selected light");break;}
    case operation::add_light: {auto lights=read_lights();if(lights.size()>=maximum_lights)throw std::runtime_error("Maximum 25 native lights");light l;l.position=at<vec3>(session,0x981a68);auto angles=at<vec3>(session,0x981a74);constexpr float rad=.01745329252f;l.look_x=l.position.x+100*cosf(angles.x*rad)*cosf(angles.y*rad);l.look_y=l.position.y+100*cosf(angles.x*rad)*sinf(angles.y*rad);l.look_z=l.position.z-100*sinf(angles.x*rad);lights.push_back(l);write_lights(lights);note("Light added at the camera");break;}
    case operation::delete_light: {auto lights=read_lights();if(r.index<0 || r.index>=int(lights.size()))throw std::runtime_error("Select a light first");lights.erase(lights.begin()+r.index);write_lights(lights);note("Light deleted");break;}
    case operation::save_cfg:save_file(filename(r),serialize(capture_controls(state.values)));refresh_files();note("Saved CFG: "+r.name);break;
    case operation::load_cfg: {
        config c=deserialize(read_file(filename(r)));
        if(!state.misc_ready || !state.sun_ready || !state.lights_ready || (c.values.fog_enabled && !state.fog_ready) || (c.values.remove_blood && !state.blood_ready))throw std::runtime_error("Film settings are not ready; use Reload after the scene appears");
        // Parse and validate the complete file before touching native state.
        if(!c.has_lights)c.values.lights=read_lights();
        state.values=c.values;apply_fog();sun_owned=true;apply_misc();write_lights(c.values.lights);apply_controls(c);note("Loaded CFG: "+r.name);break;
    }
    case operation::save_fog:save_file(filename(r),serialize_fog(state.values.mist));refresh_files();note("Saved fog preset: "+r.name);break;
    case operation::load_fog:state.values.mist=deserialize_fog(read_file(filename(r)));state.values.fog_enabled=true;apply_fog();note("Loaded fog preset: "+r.name);break;
    case operation::random_fog: {
        static std::mt19937 random{std::random_device{}()};auto f=[](std::mt19937& g,float a,float b){return std::uniform_real_distribution<float>(a,b)(g);};
        state.values.mist.base_distance=f(random,0,1000);state.values.mist.half_distance=f(random,300,10000);
        state.values.mist.tint={f(random,0,1),f(random,0,1),f(random,0,1)};state.values.mist.haze_tint={f(random,0,1),f(random,0,1),f(random,0,1)};
        state.values.mist.density=f(random,.1f,5);state.values.fog_enabled=true;apply_fog();note("Generated a random fog preset");break;
    }
    case operation::save_lights:save_file(filename(r),serialize_lights(read_lights()));refresh_files();note("Saved lighting: "+r.name);break;
    case operation::load_lights:write_lights(deserialize_lights(read_file(filename(r))));note("Loaded lighting: "+r.name);break;
    case operation::toggle_useful_dvars: {
        constexpr const char* names[]={"r_DedicatedPlayerShadowCull","r_DedicatedPlayerSunShadowResolution","demo_dollycamHighlightThreshholdDistance"};
        bool enable=r.value!=0;
        if(enable==state.useful_dvars)break;
        if(enable) {
            if(!state.useful_dvars_ready)find_dvars();
            if(!state.useful_dvars_ready)throw std::runtime_error("A required cinematic dvar is unavailable; see MVM/theater.log");
            try {write_useful_dvar(names[0],0);write_useful_dvar(names[1],0);write_useful_dvar(names[2],3);}
            catch(...) {for(auto name:names)restore_dvar(name);throw;}
            state.useful_dvars=true;note("Useful dvars enabled");
        } else {
            for(auto name:names)restore_dvar(name);
            state.useful_dvars=false;note("Useful dvars restored to their previous values");
        }
        break;
    }
    case operation::set_timescale: {auto c=capture_controls(state.values);c.timescale=r.value;apply_controls(c);note("Timescale applied");break;}
    }
}
}
bool initialize(uintptr_t game) {
    base=game;
    state.blood_ready=accessible(base+blood_address,sizeof(blood_code)) && !std::memcmp(reinterpret_cast<void*>(base+blood_address),blood_code,sizeof(blood_code));
    if(accessible(base+fog_copy,sizeof(fog_code)) && !std::memcmp(reinterpret_cast<void*>(base+fog_copy),fog_code,sizeof(fog_code)) &&
        MH_CreateHook(reinterpret_cast<void*>(base+fog_copy),copy_fog,reinterpret_cast<void**>(&fog_original))==MH_OK) {
        fog_hook=MH_EnableHook(reinterpret_cast<void*>(base+fog_copy))==MH_OK;
    }
    log_message(std::string("Scene editor fingerprints: fog=")+(fog_hook?"verified":"unavailable")+", blood="+(state.blood_ready?"verified":"unavailable"));return fog_hook;
}
void begin(uintptr_t p) {
    std::lock_guard lock(mutex);session=p;state.ready=true;state.values={};state.useful_dvars=false;
    state.sun_ready=accessible(base+sun_address,sizeof(sun),true);
    sun_color_captured=false;
    if(state.sun_ready) {
        state.values.sunlight=at<sun>(base+sun_address);
        sun_color_captured=has_sun_color(state.values.sunlight.tint);
        state.values.sunlight.tint=visible_sun_color(state.values.sunlight.tint);
    }
    find_dvars();state.values.exposure=read_float("r_exposureValue",10);state.values.sky_rotation=read_float("r_skyRotation",0);
    state.values.sky_transition=read_float("r_skyTransition",0)>0;state.values.misc_enabled=false;
    state.values.gun={read_float("cg_gun_x",0),read_float("cg_gun_y",0),read_float("cg_gun_z",0)};
    state.values.remove_gun=!read_bool("cg_drawGun",true);state.values.draw_2d=read_bool("cg_draw2d",true);
    state.values.remove_hud=at<bool>(session,0x68abbd);state.values.lights=read_lights();baseline=state.values;
    original_controls=capture_controls(state.values);
    {std::lock_guard fog_lock(fog_mutex);active=true;observed=false;fog_enabled=false;}state.fog_ready=false;
    try {refresh_files();}catch(const std::exception& e){note(e.what());}
}
void end() {
    std::lock_guard lock(mutex);
    {std::lock_guard fog_lock(fog_mutex);active=false;}
    if(session)restore(false);pending.clear();session=0;state.ready=false;state.fog_ready=false;state.useful_dvars_ready=false;
}
void set_capture_frame_rate(int fps){capture_fps=std::clamp(fps,0,240);}
bool capture_frame_lock_active(){return fixed_step_active.load();}
void shutdown() {end();if(fog_hook) {MH_DisableHook(reinterpret_cast<void*>(base+fog_copy));MH_RemoveHook(reinterpret_cast<void*>(base+fog_copy));fog_hook=false;}}
void update() {
    std::lock_guard lock(mutex);if(!session)return;
    if(state.sun_ready && !sun_color_captured && !sun_owned) {
        sun live=at<sun>(base+sun_address);
        if(has_sun_color(live.tint)) {
            state.values.sunlight=live;
            baseline.sunlight=live;
            sun_color_captured=true;
        }
    }
    int requested=capture_fps.load();
    if(requested!=applied_capture_fps){
        if(applied_capture_fps)restore_dvar("com_fixedtime_float");
        applied_capture_fps=0;fixed_step_active=false;
        if(requested && dvars.contains("com_fixedtime_float"))try{
            write_dvar("com_fixedtime_float",1000.f/float(requested));
            applied_capture_fps=requested;fixed_step_active=true;
        }catch(const std::exception& e){note(std::string("Capture frame lock unavailable: ")+e.what());}
    }
    {std::lock_guard fog_lock(fog_mutex);if(!state.fog_ready && observed && fog_hook) {state.values.mist=observed_fog;baseline.mist=observed_fog;state.fog_ready=true;}}
    state.values.lights=read_lights();state.values.remove_hud=at<bool>(session,0x68abbd);
    if(!state.values.remove_hud)restore_dvar("cg_drawCrosshair");
    auto work=std::move(pending);pending.clear();for(auto& r:work)try {execute(r);}catch(const std::exception& e) {note(std::string("Could not apply: ")+e.what());}
    if(sun_owned && state.sun_ready)at<sun>(base+sun_address)=state.values.sunlight;
    clear_blood();
    if(misc_owned)try {
        write_dvar("r_exposureTweak",1,true);write_dvar("r_exposureValue",state.values.exposure);
        write_dvar("r_skyRotation",state.values.sky_rotation);write_dvar("r_skyTransition",state.values.sky_transition?1.f:0.f);
        write_dvar("cg_gun_x",state.values.gun.x);write_dvar("cg_gun_y",state.values.gun.y);write_dvar("cg_gun_z",state.values.gun.z);
    }catch(const std::exception& e) {
        misc_owned=false;state.values.misc_enabled=false;
        for(auto name:{"r_exposureTweak","r_exposureValue","r_skyRotation","r_skyTransition","cg_gun_x","cg_gun_y","cg_gun_z"})restore_dvar(name);
        note(std::string("Scene overrides stopped: ")+e.what());
    }
}
snapshot current() {std::lock_guard lock(mutex);return state;}
void enqueue(request value) {std::lock_guard lock(mutex);if(state.ready && pending.size()<64)pending.push_back(std::move(value));}
}
