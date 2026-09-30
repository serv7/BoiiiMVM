#include "backend.hpp"
#include "build_profile.hpp"
#include "native_calls.hpp"
#include "camera_input.hpp"
#include "editor.hpp"
#include "capture.hpp"
#include <MH/MinHook.h>
#include <mutex>
#include <deque>
#include <map>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdio>

namespace theater {
std::atomic<bool> menu_open=false,hud_visible=true;
std::atomic<bool> exit_dialog=false;
namespace {
uintptr_t base=0,session=0;
std::recursive_mutex mutex;
snapshot state;
options settings;
struct request { action type; int index; std::optional<lens> optics; float wheel=0; bool zoom=false; };
std::deque<request> pending;
std::map<int,lens> extras;
lens current_lens;
void (*main_original)()=nullptr;
void (*camera_original)()=nullptr;
void (*angles_transfer_original)(const void*,void*)=nullptr;
void (*movement_original)(void*)=nullptr;
movement_scaler movement;
std::atomic<unsigned> lift_keys=0;
bool valid=false,owned=false,old_native_hidden=false,old_game_hidden=false;
bool input_owned=false,old_block=false,repositioning=false,manual_lens=false;
unsigned char reset_bytes[10]={0xc7,0x80,0x7c,0x1a,0x98,0,0,0,0,0};
bool patched=false;
std::string message="Theater controls ready";
template<class T> T& field(uintptr_t p,size_t offset) { return *reinterpret_cast<T*>(p+offset); }
bool readable(uintptr_t p,size_t n=1) {
    MEMORY_BASIC_INFORMATION m{};
    return p && VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) &&
        m.State==MEM_COMMIT && !(m.Protect&(PAGE_NOACCESS|PAGE_GUARD)) &&
        p+n<=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
}
template<class F> F function(profile::fingerprint f) { return reinterpret_cast<F>(base+f.rva); }
void command(const std::string& text) {
    function<void(*)(int,int,const char*,bool)>(profile::execute_command)(0,0,text.c_str(),false);
    log_message("Command: "+text);
}
void free_mode(int mode) {
    if(mode==2)manual_lens=false;
    set_diagnostic_stage("switch_view");
    native_calls::invoke(function<void(*)(int)>(profile::switch_view),2);
    set_diagnostic_stage("switch_free_mode");
    native_calls::invoke(function<void(*)(int,bool)>(profile::switch_free_mode),mode,false);
    set_diagnostic_stage("backend_update");
}
bool playing() {
    return valid && function<bool(*)()>(profile::demo_playing)() &&
        readable(base+profile::theater_pointer,8) &&
        readable(field<uintptr_t>(base,profile::theater_pointer)+0x9845e0,4);
}
void release_session() {
    editor::end();
    if(owned && readable(session+0x68abbe)) {
        field<bool>(session,0x68abbe)=old_native_hidden;
        field<bool>(session,0x68abbd)=old_game_hidden;
    }
    if(input_owned && readable(base+profile::block_input)) field<bool>(base,profile::block_input)=old_block;
    input_owned=false; owned=false; session=0; repositioning=false;manual_lens=false;
    menu_open=false;exit_dialog=false; extras.clear();state.editing_camera=false;
    movement.reset();
    lift_keys=0;
}
std::vector<marker> read_cameras() {
    std::vector<marker> result;
    int count=field<int>(session,0x9845d8);
    if(count<0 || count>50 || !readable(session+0x983248,count*0x64)) return result;
    for(int i=0;i<count;++i) {
        uintptr_t p=session+0x983248+i*0x64;
        marker m; m.tick=field<int>(p,0); m.position=field<vec3>(p,8); m.direction=field<vec3>(p,20);
        auto it=extras.find(m.tick); m.optics=it==extras.end()?current_lens:it->second;
        result.push_back(m);
    }
    return result;
}
void set_lens(lens value) {
    current_lens=value;
    current_lens.roll=wrap_angle(value.roll);
    current_lens.fov=std::clamp(value.fov,.1,75.);
    current_lens.focus=std::max(value.focus,.01f);
    current_lens.aperture=std::max(value.aperture,.1f);
}
void save_path() {
    const auto cams=read_cameras();
    std::filesystem::create_directories("MVM/Theater");
    // Temporary file avoids truncating a good path if saving is interrupted.
    std::ofstream out("MVM/Theater/camera-path.tmp",std::ios::binary);
    out<<"BO3_THEATER_PATH 1\n"<<cams.size()<<'\n';
    for(auto& m:cams) out<<m.tick<<' '<<m.position.x<<' '<<m.position.y<<' '<<m.position.z<<' '
        <<m.direction.x<<' '<<m.direction.y<<' '<<m.direction.z<<' '<<m.optics.roll<<' '
        <<m.optics.fov<<' '<<m.optics.focus<<' '<<m.optics.aperture<<'\n';
    out.flush(); if(!out) {message="Could not save camera path";return;} out.close();
    std::error_code ec;
    std::filesystem::copy_file("MVM/Theater/camera-path.tmp","MVM/Theater/camera-path.txt",
        std::filesystem::copy_options::overwrite_existing,ec);
    message=ec?"Could not save camera path":"Saved MVM/Theater/camera-path.txt";
}
void load_path() {
    std::ifstream in("MVM/Theater/camera-path.txt");
    std::string magic; int version=0,count=0;
    in>>magic>>version>>count;
    if(magic!="BO3_THEATER_PATH" || version!=1 || count<0 || count>50) {message="Invalid camera path";return;}
    std::vector<marker> cams(count);
    for(auto& m:cams) {
        in>>m.tick>>m.position.x>>m.position.y>>m.position.z>>m.direction.x>>m.direction.y>>m.direction.z
          >>m.optics.roll>>m.optics.fov>>m.optics.focus>>m.optics.aperture;
        if(!in || m.tick<0 || !std::isfinite(m.position.x) || !std::isfinite(m.position.y) ||
           !std::isfinite(m.position.z) || !std::isfinite(m.direction.x) || !std::isfinite(m.direction.y) ||
           !std::isfinite(m.direction.z) || !std::isfinite(m.optics.roll) || !std::isfinite(m.optics.fov) ||
           !std::isfinite(m.optics.focus) || !std::isfinite(m.optics.aperture)) {message="Invalid camera data";return;}
    }
    if(!std::is_sorted(cams.begin(),cams.end(),[](auto&a,auto&b){return a.tick<b.tick;})) {message="Camera times must be ordered";return;}
    // Native records contain additional flags. Only replace an existing path of equal size.
    auto existing=read_cameras();
    if(existing.size()!=cams.size()) {message="Create the same number of native cameras before importing this path";return;}
    for(int i=0;i<count;++i) {
        uintptr_t p=session+0x983248+i*0x64;
        field<int>(p,0)=cams[i].tick; field<vec3>(p,8)=cams[i].position;
        field<vec3>(p,20)=cams[i].direction; extras[cams[i].tick]=cams[i].optics;
    }
    message="Imported camera coordinates and lens values";
}
void execute(request r) {
    if(r.optics) {set_lens(*r.optics);manual_lens=true;return;}
    if(r.wheel) {
        // Wheel edits start from the lens currently sampled on a dolly, too.
        if(!manual_lens && field<int>(session,0x7dae64)==2 && !extras.empty())
            current_lens=sample_lens(read_cameras(),field<int>(base,profile::tick));
        if(r.zoom)current_lens.fov=zoom_fov(current_lens.fov,r.wheel,settings.fov_step);
        else current_lens.roll=wrap_angle(current_lens.roll+r.wheel*settings.roll_step);
        manual_lens=true;
        return;
    }
    auto cams=read_cameras();
    int index=r.index>=0?r.index:state.selected;
    switch(r.type) {
    case action::toggle_game_hud: command("demo_togglegamehud"); break;
    case action::cycle_view:
        log_message("F3 view "+std::to_string(field<int>(session,0x7dae60))+" -> "+std::to_string((field<int>(session,0x7dae60)+1)%3));
        set_diagnostic_stage("switch_view");
        native_calls::invoke(function<void(*)(int)>(profile::switch_view),(field<int>(session,0x7dae60)+1)%3);
        set_diagnostic_stage("backend_update");log_message("F3 native setter returned");break;
    case action::free_camera: free_mode(1); break;
    case action::dolly_camera:
        if(cams.size()<2) {message="Place at least two cameras for dolly playback";break;}
        free_mode(2); break;
    case action::interact:
        if(state.hovered>=0 && !repositioning) {
            state.selected=state.hovered; menu_open=true;state.editing_camera=true;
            message="Camera selected: choose Reposition or Delete";
        } else if(repositioning) {
            command("demo_applynewdollycammarkerposition 1");
            if(index>=0 && index<int(cams.size())) extras[cams[index].tick]=current_lens;
            repositioning=false; message="Camera repositioned";
        } else {
            free_mode(1);
            int before=field<int>(session,0x9845d8);
            command("demo_adddollycammarker");
            if(field<int>(session,0x9845d8)>before) {
                extras[field<int>(base,profile::tick)]=current_lens;
                message="Camera placed";
            } else message="Camera not placed: use a different demo time or inspect the log";
        }
        break;
    case action::select_marker: state.selected=index; break;
    case action::erase:
        if(index<0 || index>=int(cams.size())) break;
        command("demo_removedollycammarker "+std::to_string(index));
        if(field<int>(session,0x9845d8)<int(cams.size())) {
            extras.erase(cams[index].tick); state.selected=-1; message="Camera deleted";
        } else message="Native camera deletion was not accepted";
        state.editing_camera=false;menu_open=false;break;
    case action::erase_all: {
        free_mode(1); repositioning=false;
        int remaining=int(cams.size());
        // Removing the last record each time avoids invalidating subsequent indices.
        while(remaining>0 && remaining<=50) {
            command("demo_removedollycammarker "+std::to_string(remaining-1));
            int after=field<int>(session,0x9845d8);
            if(after!=remaining-1)break;
            extras.erase(cams[remaining-1].tick); remaining=after;
        }
        state.selected=-1;
        if(remaining==0)extras.clear();
        message=remaining==0?"All cameras deleted":"Native camera deletion stopped; cameras remain";
        break;
    }
    case action::reposition:
        if(index<0 || index>=int(cams.size())) break;
        free_mode(1); command("demo_repositiondollycammarker "+std::to_string(index));
        set_lens(cams[index].optics); repositioning=true; state.selected=index;
        menu_open=false;state.editing_camera=false; message="Move the camera, then press F to apply"; break;
    case action::apply:
        if(index>=0 && index<int(cams.size())) {extras[cams[index].tick]=current_lens;manual_lens=false;message="Lens values saved to camera";}
        break;
    case action::seek_back: case action::seek_forward: case action::seek_to: {
        manual_lens=false;
        int amount=std::max(1,int(settings.seek_seconds*1000));
        // BO3's optional argument is an absolute demo tick, not a duration.
        int target=r.type==action::seek_to?r.index:std::max(0,state.tick+(r.type==action::seek_back?-amount:amount));
        if(state.end_tick>state.start_tick)target=std::clamp(target,state.start_tick,state.end_tick-1);
        if(target!=state.tick)command(std::string(target<state.tick?"demo_back ":"demo_forward ")+std::to_string(target));
        break;
    }
    case action::speed_down: case action::speed_up: {
        float speed=step_speed(field<float>(session,8),r.type==action::speed_up?1:-1);
        // Bypass only the command's 0.1 lower bound. Use the native setter for all speeds.
        function<void(*)(float)>(profile::set_timescale)(speed);
        message="Playback speed changed"; break;
    }
    case action::first_camera:
        if(cams.size()<2) {message="Place at least two cameras first";break;}
        free_mode(1); command("demo_switchdollycammarker 0"); free_mode(2);
        message="Jumped to first camera / Dolly"; break;
    case action::pause: command("demo_pause"); break;
    case action::exit_film:
        menu_open=false;exit_dialog=false;capture::stop();
        command("LobbyStopDemo");message="Requested exit to main menu";break;
    case action::save_path: save_path();break;
    case action::load_path: load_path();break;
    default: break;
    }
}
void update() {
    std::lock_guard lock(mutex);
    if(!playing()) {
        if(owned) release_session();
        pending.clear(); state.playing=false; return;
    }
    uintptr_t new_session=field<uintptr_t>(base,profile::theater_pointer);
    if(session!=new_session) {
        release_session(); session=new_session; owned=true;
        old_native_hidden=field<bool>(session,0x68abbe);
        old_game_hidden=field<bool>(session,0x68abbd);
        current_lens.roll=field<float>(session,0x981a7c);
        current_lens.fov=field<double>(base,profile::fov);
        uintptr_t cg=field<uintptr_t>(base,profile::camera_pointer);
        if(readable(cg+0x131d34,4)) {
            current_lens.focus=field<float>(cg,0x131d30);
            current_lens.aperture=field<float>(cg,0x131d34);
        }
        for(const auto& camera:read_cameras())extras[camera.tick]=current_lens;
        state.selected=-1;
        editor::begin(session);
    }
    field<bool>(session,0x68abbe)=true;
    bool capture=menu_open.load() || exit_dialog.load();
    if(capture && !input_owned) {old_block=field<bool>(base,profile::block_input);input_owned=true;}
    if(input_owned) {field<bool>(base,profile::block_input)=capture?true:old_block;if(!capture)input_owned=false;}
    state.tick=field<int>(base,profile::tick);
    function<void(*)(int*,int*)>(profile::timeline_bounds)(&state.start_tick,&state.end_tick);
    state.paused=function<bool(*)()>(profile::demo_paused)();
    state.eye=field<vec3>(session,0x981a68);state.angles=field<vec3>(session,0x981a74);
    state.cameras=read_cameras();
    state.hovered=hovered_marker(state.cameras,state.eye,state.angles,float(current_lens.fov));
    auto work=std::move(pending);pending.clear();
    for(auto& r:work) execute(r);
    editor::update();
    state.available=valid; state.playing=true; state.speed=field<float>(session,8);
    state.view=field<int>(session,0x7dae60);state.free_mode=field<int>(session,0x7dae64);
    state.game_hud=!field<bool>(session,0x68abbd);state.native_hud=!field<bool>(session,0x68abbe);
    state.cameras=read_cameras();
    state.optics=state.view==2 && state.free_mode==2 && !extras.empty() && !manual_lens?sample_lens(state.cameras,state.tick):current_lens;
    state.repositioning=repositioning;
    state.message=message;
}
void main_frame() {set_diagnostic_stage("backend_update");update();set_diagnostic_stage("idle");main_original();}
void camera_frame() {
    set_diagnostic_stage("camera_lens");
    // Applied before the game's angle-to-axis conversion, including paused demos.
    if(playing()) {
        std::lock_guard lock(mutex);
        uintptr_t p=field<uintptr_t>(base,profile::theater_pointer);
        if(p==session) {
            lens value=current_lens;
            if(field<int>(p,0x7dae64)==2 && !extras.empty() && !manual_lens) {
                auto cams=read_cameras();
                value=sample_lens(cams,field<int>(base,profile::tick));
            }
            field<float>(p,0x981a7c)=value.roll;
            field<double>(base,profile::fov)=value.fov;
            uintptr_t cg=field<uintptr_t>(base,profile::camera_pointer);
            if(readable(cg+0x131d34,4)) {
                field<float>(cg,0x131d30)=value.focus;
                field<float>(cg,0x131d34)=value.aperture;
            }
        }
    }
    static bool first=true;
    if(first) {log_message("First free-camera update: FOV="+std::to_string(current_lens.fov)+", roll="+std::to_string(current_lens.roll));first=false;}
    set_diagnostic_stage("camera_original");native_calls::invoke(camera_original);set_diagnostic_stage("idle");
}
void camera_angles_transfer(const void* source,void* destination) {
    // BO3 has finished its own dolly update at this call site. Curve only the
    // position before the renderer copies it. Marker +20 is a direction vector,
    // not Euler angles; BO3 must retain control of orientation and roll.
    if(valid && session && source==reinterpret_cast<const void*>(session+0x981a74)) {
        std::lock_guard lock(mutex);
        if(field<int>(session,0x7dae60)==2 && field<int>(session,0x7dae64)==2) {
            auto cameras=read_cameras();
            if(auto position=sample_dolly_position(cameras,field<int>(base,profile::tick)))
                field<vec3>(session,0x981a68)=*position;
        }
    }
    angles_transfer_original(source,destination);
}
void camera_movement(void* cmd) {
    if(!playing()) {native_calls::invoke(movement_original,cmd);return;}
    std::lock_guard lock(mutex);
    uintptr_t p=field<uintptr_t>(base,profile::theater_pointer);
    int mode=field<int>(p,0x7dae64);
    if(p!=session || field<int>(p,0x7dae60)!=2 || (mode!=0 && mode!=1)) {
        movement.reset();native_calls::invoke(movement_original,cmd);return;
    }
    vec3 before=field<vec3>(p,0x981a68),angles=field<vec3>(p,0x981a74);
    bool capture=menu_open.load();
    if(capture)lift_keys=0;
    unsigned lift=lift_keys.load();
    set_diagnostic_stage("camera_movement");
    if(lift && !capture && readable(reinterpret_cast<uintptr_t>(cmd),16)) {
        // Temporarily map Q/E into the engine's keyboard/controller vertical
        // buttons. Native acceleration and all three scaled axes stay together.
        camera_lift_scope input(cmd,lift);
        native_calls::invoke(movement_original,cmd);
    } else native_calls::invoke(movement_original,cmd);
    if(field<int>(p,0x7dae60)!=2 || field<int>(p,0x7dae64)!=mode) {movement.reset();set_diagnostic_stage("idle");return;}
    capture=menu_open.load();
    field<vec3>(p,0x981a68)=movement.apply(before,field<vec3>(p,0x981a68),capture?0.f:settings.move_speed);
    if(capture) {
        field<vec3>(p,0x981a74)=angles;
        field<vec3>(p,0x981a80)={};
    }
    set_diagnostic_stage("idle");
}
}
void log_message(const std::string& text) {
    static std::mutex log_mutex; std::lock_guard lock(log_mutex);
    std::ofstream("MVM/theater.log",std::ios::app)<<GetTickCount64()<<" "<<text<<'\n';
}
bool initialize_backend() {
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"BlackOps3.exe"));
    if(!base) {log_message("BlackOps3.exe module unavailable");return false;}
    const profile::fingerprint checks[]={profile::set_timescale,profile::seek_relative,
        profile::toggle_demo_hud,profile::toggle_game_hud,profile::demo_playing,
        profile::add_marker_cmd,profile::reposition_marker_cmd,profile::remove_marker_cmd,
        profile::apply_marker_cmd,profile::switch_view,profile::switch_free_mode,
        profile::execute_command,profile::camera_frame,profile::camera_angles_transfer,profile::jump_marker,
        profile::timeline_bounds,profile::demo_paused,profile::camera_movement,
        profile::camera_vertical_up,profile::camera_vertical_down,profile::camera_vertical_controller};
    for(auto f:checks) {
        if(!readable(base+f.rva,f.length) || std::memcmp(reinterpret_cast<void*>(base+f.rva),f.bytes,f.length)) {
            char buf[100];std::snprintf(buf,sizeof(buf),"Build mismatch at RVA %zx: controls disabled",size_t(f.rva));
            log_message(buf); std::lock_guard lock(mutex);state.message=buf;return false;
        }
    }
    if(std::memcmp(reinterpret_cast<void*>(base+profile::roll_reset),reset_bytes,10)) {log_message("Roll reset mismatch");return false;}
    if(!native_calls::initialize(base)) {log_message("BOIII native-call proxy mismatch: controls disabled");return false;}
    log_message("BOIII native-call proxy verified; protected camera calls enabled");
    // The client already detours Com_Frame; MinHook chains its current entry point.
    if(!readable(base+profile::main_frame,16)) return false;
    auto status=MH_Initialize();
    if(status!=MH_OK && status!=MH_ERROR_ALREADY_INITIALIZED)return false;
    editor::initialize(base);
    if(MH_CreateHook(reinterpret_cast<void*>(base+profile::main_frame),main_frame,reinterpret_cast<void**>(&main_original))!=MH_OK)return false;
    if(MH_CreateHook(reinterpret_cast<void*>(base+profile::camera_frame.rva),camera_frame,reinterpret_cast<void**>(&camera_original))!=MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::main_frame));return false;
    }
    if(MH_CreateHook(reinterpret_cast<void*>(base+profile::camera_movement.rva),camera_movement,reinterpret_cast<void**>(&movement_original))!=MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::camera_frame.rva));
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::main_frame));return false;
    }
    if(MH_CreateHook(reinterpret_cast<void*>(base+profile::camera_angles_transfer.rva),camera_angles_transfer,reinterpret_cast<void**>(&angles_transfer_original))!=MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::camera_movement.rva));
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::camera_frame.rva));
        MH_RemoveHook(reinterpret_cast<void*>(base+profile::main_frame));return false;
    }
    DWORD old; auto reset=reinterpret_cast<void*>(base+profile::roll_reset);
    if(!VirtualProtect(reset,10,PAGE_EXECUTE_READWRITE,&old))return false;
    std::memset(reset,0x90,10);VirtualProtect(reset,10,old,&old);FlushInstructionCache(GetCurrentProcess(),reset,10);patched=true;
    valid=true;
    MH_EnableHook(reinterpret_cast<void*>(base+profile::camera_frame.rva));
    MH_EnableHook(reinterpret_cast<void*>(base+profile::camera_movement.rva));
    MH_EnableHook(reinterpret_cast<void*>(base+profile::camera_angles_transfer.rva));
    MH_EnableHook(reinterpret_cast<void*>(base+profile::main_frame));
    {std::lock_guard lock(mutex);state.available=true;}
    log_message("Exact game build verified; theater backend enabled");return true;
}
void shutdown_backend() {
    if(base) {
        MH_DisableHook(reinterpret_cast<void*>(base+profile::main_frame));
        MH_DisableHook(reinterpret_cast<void*>(base+profile::camera_frame.rva));
        MH_DisableHook(reinterpret_cast<void*>(base+profile::camera_movement.rva));
        MH_DisableHook(reinterpret_cast<void*>(base+profile::camera_angles_transfer.rva));
    }
    std::lock_guard lock(mutex);valid=false;release_session();editor::shutdown();
    if(patched) {
        DWORD old;auto reset=reinterpret_cast<void*>(base+profile::roll_reset);
        if(VirtualProtect(reset,10,PAGE_EXECUTE_READWRITE,&old)) {
            std::memcpy(reset,reset_bytes,10);VirtualProtect(reset,10,old,&old);FlushInstructionCache(GetCurrentProcess(),reset,10);
        }
        patched=false;
    }
}
void enqueue(action type,int index) {std::lock_guard lock(mutex);if(state.playing && pending.size()<64)pending.push_back({type,index});}
void enqueue_lens(lens value) {std::lock_guard lock(mutex);if(state.playing && pending.size()<64)pending.push_back({action::apply,-1,value});}
void enqueue_roll(float wheel) {std::lock_guard lock(mutex);if(state.playing && state.view==2 && pending.size()<64)pending.push_back({action::apply,-1,{},wheel});}
void enqueue_zoom(float wheel) {std::lock_guard lock(mutex);if(state.playing && state.view==2 && pending.size()<64)pending.push_back({action::apply,-1,{},wheel,true});}
snapshot current_snapshot(){std::lock_guard lock(mutex);return state;}
options current_options(){std::lock_guard lock(mutex);return settings;}
void set_options(options value){std::lock_guard lock(mutex);value.move_speed=camera_speed(value.move_speed);settings=value;}
void set_camera_lift_key(bool up,bool held) {
    unsigned bit=up?1u:2u;
    if(held)lift_keys.fetch_or(bit);else lift_keys.fetch_and(~bit);
}
void clear_camera_lift(){lift_keys=0;}
void close_camera_editor(){std::lock_guard lock(mutex);state.editing_camera=false;}
namespace editor {
config capture_controls(scene values) {
    std::lock_guard lock(mutex);config c;c.values=std::move(values);c.move_speed=settings.move_speed;c.fov=float(current_lens.fov);
    c.timescale=session?field<float>(session,8):1;c.roll_step=settings.roll_step;c.fov_step=settings.fov_step;
    c.seek_seconds=settings.seek_seconds;c.markers=settings.show_markers;c.help=settings.show_help;c.menu_background=settings.menu_background;return c;
}
void apply_controls(const config& c) {
    // Editor requests execute only during backend_update on the native thread.
    settings.move_speed=camera_speed(c.move_speed);settings.roll_step=c.roll_step;settings.fov_step=c.fov_step;
    settings.seek_seconds=c.seek_seconds;settings.show_markers=c.markers;settings.show_help=c.help;settings.menu_background=c.menu_background;
    auto optics=current_lens;optics.fov=c.fov;set_lens(optics);manual_lens=true;
    if(session && std::isfinite(c.timescale))function<void(*)(float)>(profile::set_timescale)(std::clamp(c.timescale,.01f,10.f));
}
void goto_position(vec3 position) {free_mode(1);field<vec3>(session,0x981a68)=position;field<vec3>(session,0x981a80)={};movement.reset();}
}
}
