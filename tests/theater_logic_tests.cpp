#include "../src/theater_logic.hpp"
#include "../src/camera_input.hpp"
#include <cassert>
int main() {
    using namespace theater;
    camera_buttons original={0x84210001u,0x12345678u,0x89abcdefu};
    auto up=lift_buttons(original,1),down=lift_buttons(original,2),both=lift_buttons(original,3),none=lift_buttons(original,0);
    assert((up.main&(3u<<16))==(1u<<17) && (up.controller_high&(1u<<31)) && !(up.controller_low&2));
    assert((down.main&(3u<<16))==(1u<<16) && !(down.controller_high&(1u<<31)) && (down.controller_low&2));
    assert(!(both.main&(3u<<16)) && !(both.controller_high&(1u<<31)) && !(both.controller_low&2));
    assert(up.main==(original.main&~(3u<<16))+(1u<<17));
    assert((up.controller_high&~(1u<<31))==(original.controller_high&~(1u<<31)));
    assert(none.main==original.main && none.controller_low==original.controller_low && none.controller_high==original.controller_high);
    std::array<unsigned char,0x60> cmd{};cmd.fill(0x5a);std::memcpy(cmd.data()+4,&original,sizeof(original));auto untouched=cmd;
    {camera_lift_scope input(cmd.data(),1);camera_buttons pressed;std::memcpy(&pressed,cmd.data()+4,sizeof(pressed));assert(pressed.main==up.main);assert(cmd[0]==0x5a && cmd[16]==0x5a);}
    assert(cmd==untouched); // Native command packet is unchanged after the call.
    assert(camera_speed(0)==.0001f && camera_speed(20)==10);
    assert(camera_speed(std::numeric_limits<float>::quiet_NaN())==1);
    movement_scaler movement;
    auto p=movement.apply({1000,2000,3000},{1010,2020,3030},.1f);
    assert(p.x==1001 && p.y==2002 && p.z==3003);
    p=movement.apply(p,{1020,2040,3060},0);assert(p.x==1001 && p.y==2002 && p.z==3003);
    p=movement.apply({50,60,70},{60,80,100},1);assert(p.x==60 && p.y==80 && p.z==100);
    movement.reset();p={1000,0,0};
    for(int i=0;i<1000;++i)p=movement.apply(p,{p.x+.01f,0,0},.0001f);
    assert(p.x>1000.0009f && p.x<1000.0011f);
    movement.reset();p={0,0,3000};
    for(int i=0;i<1000;++i)p=movement.apply(p,{0,0,p.z+.01f},.0001f);
    assert(double(p.z)-3000.>.0009 && double(p.z)-3000.<.0011);
    movement.reset();p=movement.apply({0,0,100},{0,0,90},.1f);assert(p.z==99);
    p=movement.apply(p,{0,0,109},0);assert(p.z==99);
    assert(zoom_fov(65,1,2)==63);
    assert(zoom_fov(65,-1,2)==67);
    assert(zoom_fov(.2,1,2)==.1);
    assert(zoom_fov(74,-1,2)==75);
    assert(timeline_fraction(1500,1000,2000)==.5f);
    assert(timeline_fraction(0,1000,2000)==0);
    assert(timeline_fraction(3000,1000,2000)==1);
    assert(timeline_fraction(1000,1000,1000)==0);
    assert(timeline_tick(.5f,1000,2000)==1500);
    assert(timeline_tick(-1,1000,2000)==1000);
    assert(timeline_tick(2,1000,2000)==2000);
    assert(timeline_tick(.5f,1000,1000)==1000);
    assert(step_speed(.01f,-1)==.01f);
    assert(step_speed(10.f,1)==10.f);
    assert(step_speed(1.f,1)==1.25f);
    assert(step_speed(.07f,-1)==.05f);
    assert(step_speed(.07f,1)==.1f);
    for(size_t i=1;i<speeds.size();++i) {
        assert(step_speed(speeds[i],-1)==speeds[i-1]);
        assert(step_speed(speeds[i-1],1)==speeds[i]);
    }
    std::vector<marker> path={{100,{100,0,0},{},{170,60,1,2}},
                              {200,{200,0,0},{},{-170,80,3,4}}};
    auto mid=sample_lens(path,150);
    assert(std::abs(mid.roll-180.f)<.001f);
    assert(mid.fov==70 && mid.focus==2 && mid.aperture==3);
    assert(sample_lens(path,0).fov==60);
    assert(sample_lens(path,1000).fov==80);
    auto eased=sample_dolly_position(path,125);
    assert(eased && eased->x>110 && eased->x<125);
    assert(sample_dolly_position(path,100)->x==100);
    assert(sample_dolly_position(path,200)->x==200);
    std::vector<marker> curved={{100,{0,0,0},{0,170,0},{}},
                                {200,{10,0,0},{0,-170,0},{}},
                                {300,{10,10,0},{0,-150,0},{}}};
    auto bend=sample_dolly_position(curved,150);
    assert(bend && bend->x>0 && bend->x<10 && bend->y<-.1f);
    curved[1].direction={100000,-100000,300000};
    auto same_bend=sample_dolly_position(curved,150);
    assert(same_bend && same_bend->x==bend->x && same_bend->y==bend->y);
    auto before=sample_dolly_position(curved,199),at=sample_dolly_position(curved,200),after=sample_dolly_position(curved,201);
    assert(before && at && after && at->x==10 && at->y==0);
    assert(std::abs((at->x-before->x)-(after->x-at->x))<.01f);
    assert(std::abs((at->y-before->y)-(after->y-at->y))<.01f);
    assert(sample_dolly_position(curved,0)->x==0);
    assert(sample_dolly_position(curved,1000)->y==10);
    curved[1].tick=100;assert(!sample_dolly_position(curved,150));
    assert(hovered_marker(path,{0,0,0},{0,0,0})==0);
    assert(hovered_marker(path,{0,0,0},{0,180,0})==-1);
    path.insert(path.begin(),{50,{1,100,0},{},{}});
    assert(hovered_marker(path,{0,0,0},{0,0,0})==1);
    std::vector<marker> narrow={{100,{100,3,0},{},{}}};
    assert(hovered_marker(narrow,{0,0,0},{0,0,0},3.f)==-1);
    assert(hovered_marker(narrow,{0,0,0},{0,0,0},65.f)==0);
    narrow[0].position={100,.1f,0};
    assert(hovered_marker(narrow,{0,0,0},{0,0,0},3.f)==0);
}
