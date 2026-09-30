#pragma once
#include <Windows.h>
#include <string>

namespace theater::binds {
enum class id { menu,controls,game_hud,view,place,delete_all,seek_back,seek_forward,
    speed_up,speed_down,pause,first_camera,lift_down,lift_up,dolly,edit,
    roll_left,roll_right,zoom_in,zoom_out,record,exit_film,count };
struct chord {int key=0;int modifiers=0;};
constexpr int mouse_left=0x100,mouse_right=0x101,wheel_up=0x102,wheel_down=0x103;
constexpr int control=1,shift=2,alt=4;
void load();
void save();
void defaults();
chord get(id action);
bool assign(id action,chord value);
id match(chord value);
const char* title(id action);
std::string label(id action);
int modifiers();
void listen(id action);
bool listening();
id listening_for();
void cancel();
bool accept(chord value);
}
