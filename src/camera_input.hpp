#pragma once
#include <cstdint>
#include <cstring>

namespace theater {
struct camera_buttons {uint32_t main,controller_low,controller_high;};
inline camera_buttons lift_buttons(camera_buttons original,unsigned held) {
    if(!held)return original;
    constexpr uint32_t up=1u<<17,down=1u<<16;
    original.main&=~(up|down);
    original.controller_low&=~(1u<<1);
    original.controller_high&=~(1u<<31);
    if(held==1) {original.main|=up;original.controller_high|=1u<<31;}
    if(held==2) {original.main|=down;original.controller_low|=1u<<1;}
    return original; // Both keys cancel the vertical input.
}
class camera_lift_scope {
    void* fields;
    camera_buttons previous;
public:
    camera_lift_scope(void* cmd,unsigned held):fields(static_cast<unsigned char*>(cmd)+4) {
        std::memcpy(&previous,fields,sizeof(previous));
        auto replacement=lift_buttons(previous,held);
        std::memcpy(fields,&replacement,sizeof(replacement));
    }
    ~camera_lift_scope() {std::memcpy(fields,&previous,sizeof(previous));}
    camera_lift_scope(const camera_lift_scope&)=delete;
    camera_lift_scope& operator=(const camera_lift_scope&)=delete;
};
}
