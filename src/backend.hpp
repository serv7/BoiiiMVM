#pragma once
#include <Windows.h>
#include <atomic>
#include <string>
#include <vector>
#include "theater_logic.hpp"
namespace theater {
enum class action { toggle_game_hud,toggle_ui,cycle_view,free_camera,dolly_camera,
    interact,erase,reposition,apply,seek_back,seek_forward,speed_down,speed_up,
    first_camera,pause,select_marker,save_path,load_path,erase_all,seek_to,exit_film };
struct snapshot {
    bool available=false,playing=false,paused=false,repositioning=false,game_hud=true,native_hud=true,editing_camera=false;
    int tick=0,start_tick=0,end_tick=0,view=0,free_mode=0,hovered=-1,selected=-1;
    float speed=1;
    vec3 eye,angles;
    lens optics;
    std::vector<marker> cameras;
    std::string message;
};
struct options {
    float roll_step=1,fov_step=1,seek_seconds=5,move_speed=1;
    bool show_markers=true,show_help=true,menu_background=true;
};
bool initialize_backend();
void shutdown_backend();
void enqueue(action type,int index=-1);
void enqueue_lens(lens value);
void enqueue_roll(float notches);
void enqueue_zoom(float notches);
void set_camera_lift_key(bool up,bool held);
void clear_camera_lift();
void close_camera_editor();
snapshot current_snapshot();
void set_options(options value);
options current_options();
extern std::atomic<bool> menu_open;
extern std::atomic<bool> hud_visible;
extern std::atomic<bool> exit_dialog;
void configure_ui();
void draw_ui();
void log_message(const std::string& text);
void set_diagnostic_stage(const char* stage);
void initialize_diagnostics();
void shutdown_diagnostics();
}
