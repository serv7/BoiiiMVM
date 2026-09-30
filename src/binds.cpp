#include "binds.hpp"
#include "vendor/json.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>

namespace theater::binds {
namespace {
constexpr size_t count=size_t(id::count);
const char* names[count]={"Open menu","Toggle controls","Toggle game HUD","Cycle camera view",
    "Place / edit camera","Delete all cameras","Seek backward","Seek forward",
    "Increase timescale","Decrease timescale","Play / pause","First camera / dolly",
    "Camera down","Camera up","Dolly camera","Edit camera",
    "Roll left","Roll right","Zoom in","Zoom out","Start / stop recording","Exit film"};
std::array<chord,count> defaults_value={{{VK_TAB,0},{VK_F4,0},{VK_F2,0},{VK_F3,0},
    {'F',0},{'L',0},{VK_LEFT,0},{VK_RIGHT,0},{VK_UP,0},{VK_DOWN,0},
    {VK_SPACE,0},{'M',control},{'Q',0},{'E',0},{mouse_left,0},{mouse_right,0},
    {wheel_down,0},{wheel_up,0},{wheel_up,control},{wheel_down,control},{VK_F5,0},{VK_F10,0}}};
std::array<chord,count> values=defaults_value;
std::recursive_mutex guard;
int waiting=-1;
const std::filesystem::path path="MVM/Theater/binds.json";
bool valid(chord c) {return c.key>0 && c.key<=wheel_down && c.modifiers>=0 && c.modifiers<=7 &&
    c.key!=VK_ESCAPE && c.key!=VK_CONTROL && c.key!=VK_SHIFT && c.key!=VK_MENU &&
    c.key!=VK_LCONTROL && c.key!=VK_RCONTROL && c.key!=VK_LSHIFT && c.key!=VK_RSHIFT &&
    c.key!=VK_LMENU && c.key!=VK_RMENU;}
}
void defaults(){std::lock_guard lock(guard);values=defaults_value;waiting=-1;save();}
void load(){
    std::lock_guard lock(guard);
    try {
        std::ifstream input(path);if(!input)return;
        nlohmann::json data;input>>data;
        if(!data.is_object() || !data.contains("binds") || !data["binds"].is_array() || data["binds"].size()!=count)return;
        std::array<chord,count> next{};
        for(size_t i=0;i<count;++i) {
            auto& item=data["binds"][i];next[i]={item.at("key").get<int>(),item.at("modifiers").get<int>()};
            if(!valid(next[i]) || ((i==size_t(id::lift_down) || i==size_t(id::lift_up)) &&
                (next[i].key==wheel_up || next[i].key==wheel_down)))return;
            for(size_t j=0;j<i;++j)if(next[i].key==next[j].key && next[i].modifiers==next[j].modifiers)return;
        }
        values=next;
    } catch(...) {values=defaults_value;}
}
void save(){
    std::lock_guard lock(guard);
    try {
        std::filesystem::create_directories(path.parent_path());nlohmann::json data;data["version"]=1;data["binds"]=nlohmann::json::array();
        for(auto v:values)data["binds"].push_back({{"key",v.key},{"modifiers",v.modifiers}});
        auto temporary=path;temporary+=".tmp";{std::ofstream output(temporary,std::ios::trunc);output<<data.dump(2)<<'\n';}
        std::error_code ec;std::filesystem::rename(temporary,path,ec);
        if(ec){std::filesystem::remove(path,ec);std::filesystem::rename(temporary,path,ec);}
    } catch(...) {}
}
chord get(id action){std::lock_guard lock(guard);return values[size_t(action)];}
bool assign(id action,chord value){
    std::lock_guard lock(guard);
    if(!valid(value))return false;
    if((action==id::lift_down || action==id::lift_up) && (value.key==wheel_up || value.key==wheel_down))return false;
    auto index=size_t(action);if(index>=count)return false;
    for(size_t i=0;i<count;++i)if(i!=index && values[i].key==value.key && values[i].modifiers==value.modifiers){
        if((i==size_t(id::lift_down) || i==size_t(id::lift_up)) &&
            (values[index].key==wheel_up || values[index].key==wheel_down))return false;
        values[i]=values[index];
    }
    values[index]=value;save();return true;
}
id match(chord value){std::lock_guard lock(guard);for(size_t i=0;i<count;++i)if(values[i].key==value.key && values[i].modifiers==value.modifiers)return id(i);return id::count;}
const char* title(id action){return names[size_t(action)];}
int modifiers(){return ((GetKeyState(VK_CONTROL)&0x8000)?control:0)|((GetKeyState(VK_SHIFT)&0x8000)?shift:0)|((GetKeyState(VK_MENU)&0x8000)?alt:0);}
std::string label(id action){
    std::lock_guard lock(guard);
    auto c=get(action);std::string value;
    if(c.modifiers&control)value+="CTRL + ";if(c.modifiers&shift)value+="SHIFT + ";if(c.modifiers&alt)value+="ALT + ";
    if(c.key==mouse_left)value+="LMB";else if(c.key==mouse_right)value+="RMB";
    else if(c.key==wheel_up)value+="WHEEL UP";else if(c.key==wheel_down)value+="WHEEL DOWN";
    else if(c.key>='A' && c.key<='Z')value+=char(c.key);
    else if(c.key>=VK_F1 && c.key<=VK_F24)value+="F"+std::to_string(c.key-VK_F1+1);
    else switch(c.key){case VK_TAB:value+="TAB";break;case VK_SPACE:value+="SPACE";break;
        case VK_LEFT:value+="LEFT";break;case VK_RIGHT:value+="RIGHT";break;
        case VK_UP:value+="UP";break;case VK_DOWN:value+="DOWN";break;
        default:value+="KEY "+std::to_string(c.key);break;}
    return value;
}
void listen(id action){std::lock_guard lock(guard);waiting=int(action);}
bool listening(){std::lock_guard lock(guard);return waiting>=0;}
id listening_for(){std::lock_guard lock(guard);return waiting>=0?id(waiting):id::count;}
void cancel(){std::lock_guard lock(guard);waiting=-1;}
bool accept(chord value){std::lock_guard lock(guard);if(waiting<0)return false;if(value.key==VK_ESCAPE){cancel();return true;}
    if(!valid(value))return false;auto target=id(waiting);waiting=-1;return assign(target,value);}
}
