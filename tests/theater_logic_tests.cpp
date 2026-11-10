#include "../src/theater_logic.hpp"
#include "../src/camera_input.hpp"
#include "../src/animation_records.hpp"
#include <cassert>
struct animation_fixture {
    std::map<std::string,std::pair<uintptr_t,theater::animations::record>> assets;
    uintptr_t find(const std::string& name){auto it=assets.find(name);return it==assets.end()?0:it->second.first;}
    bool read(const std::string& name,uintptr_t address,theater::animations::record& out){auto it=assets.find(name);if(it==assets.end() || it->second.first!=address)return false;out=it->second.second;return true;}
    bool write(uintptr_t address,const theater::animations::record& data){for(auto& [name,item]:assets)if(item.first==address){item.second=data;return true;}return false;}
};
int main() {
    using namespace theater;
    assert(modified_camera_speed(.1f,4,.1f,true,false)==.4f);
    assert(std::abs(modified_camera_speed(.1f,4,.1f,true,true)-.01f)<.00001f);
    assert(next_camera_mode(0,1)==2 && next_camera_mode(0,-1)==1);
    assert(next_camera_mode(1,1)==2 && next_camera_mode(2,1)==1);
    assert(next_camera_mode(1,-1)==2 && next_camera_mode(2,-1)==1);
    animation_fixture clips;animations::record a{},b{},c{};a.fill(1);b.fill(2);c.fill(3);
    clips.assets={{"a",{100,a}},{"b",{200,b}},{"c",{300,c}}};animations::swaps swaps;
    assert(!swaps.replace("a","a",clips));assert(!swaps.replace("a","missing",clips));
    assert(swaps.replace("a","b",clips));auto replaced=clips.assets["a"].second;
    assert(replaced[8]==2 && replaced[animations::name_offset]==1 && replaced.back()==2);
    assert(swaps.replace("b","c",clips));assert(swaps.replace("c","b",clips));assert(clips.assets["c"].second[8]==2);
    assert(swaps.restore_all(clips));assert(clips.assets["a"].second==a && clips.assets["b"].second==b && clips.assets["c"].second==c);
    assert(swaps.replace("a","b",clips));clips.assets["a"].second[10]=19;assert(!swaps.restore("a",clips));
    swaps.clear();clips.assets["a"].second=a;assert(swaps.replace("a","b",clips));clips.assets["a"].first=400;assert(!swaps.restore("a",clips));
    demo_clock clock;assert(clock.sample(1000,false,.01,0)==1000);
    auto fractional=clock.sample(1000,false,.01,16.666);assert(fractional>1000.16 && fractional<1000.17);
    assert(clock.sample(1000,true,.01,16.666)==fractional);
    assert(clock.sample(500,false,1,16.666)==500);
    assert(sample_lens({{0,{},{},{0,60,1,2}},{1000,{},{},{30,70,2,3}}},500.1).roll>15);
    std::vector<marker> looking={{0,{1000,100,200},{1100,100,200},{0,65,2,8}},
        {1000,{2000,200,300},{2000,300,300},{10,65,2,8}},
        {2000,{3000,300,400},{2900,300,400},{20,65,2,8}}};
    auto angle=sample_dolly_angles(looking,1000);assert(angle && std::abs(angle->x)<.0001 && std::abs(angle->y-90)<.0001 && angle->z==10);
    auto before_angle=sample_dolly_angles(looking,999.9),after_angle=sample_dolly_angles(looking,1000.1);
    assert(std::abs((angle->y-before_angle->y)-(after_angle->y-angle->y))<.001);
    looking[0].direction={900,101.745f,200};looking[1].direction={1900,198.255f,300};looking.resize(2);
    angle=sample_dolly_angles(looking,500);assert(angle && std::abs(angle->y-180)<.01 && std::abs(angle->x)<.001);
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
    {
        // Seek: the engine jump lands short, the rest is played fast and slowed onto the target.
        seek_controller seek;seek.start(50000,10000,false,1.f);
        double now=12000;int steps=0;seek_step last;
        for(;steps<2000 && seek.active;++steps) {
            last=seek.frame(int(now),90000,false,16.);
            if(last.type==seek_step::set_speed){assert(last.speed>=seek_controller::settle_speed && last.speed<=seek_controller::max_speed);now+=last.speed*16.;}
            assert(last.type!=seek_step::jump && !last.toggle_pause);
        }
        assert(!seek.active && last.type==seek_step::finish && last.speed==1.f && !last.toggle_pause);
        // It slows down near the target rather than flying past it.
        assert(now>=50000 && now-50000<=16);
    }
    {
        // Seek back from a paused film: the jump lands past the target, so it jumps back again; then
        // exactly one unpause, play onto the target, and exactly one pause to leave it paused.
        seek_controller seek;seek.start(20000,30000,true,.5f);
        double now=25000;bool jumped=false,paused=true;int toggles=0;seek_step last;
        for(int i=0;i<5000 && seek.active;++i) {
            last=seek.frame(int(now),90000,paused,16.);
            if(last.toggle_pause){++toggles;paused=!paused;}
            if(last.type==seek_step::jump){assert(last.tick==20000);jumped=true;now=19000;}
            else if(last.type==seek_step::set_speed && !paused)now+=last.speed*16.;
        }
        assert(jumped && last.type==seek_step::finish && toggles==2 && paused && last.speed==.5f && now>=20000);
    }
    {
        // If demo_pause doesn't take, it is not sent again: one toggle, then it stays where it landed.
        seek_controller seek;seek.start(30000,10000,true,1.f);
        int toggles=0,frames=0;seek_step last;
        while(seek.active && frames<1000){last=seek.frame(15000,90000,true,16.);toggles+=last.toggle_pause;++frames;}
        assert(!seek.active && toggles==1 && !last.toggle_pause && frames<=seek_controller::settle_frames+seek_controller::unpause_frames+3);
    }
    {
        // An exact engine jump finishes after settling; a stuck film gives up instead of looping.
        seek_controller exact;exact.start(40000,10000,false,2.f);seek_step last;
        for(int i=0;i<10 && exact.active;++i)last=exact.frame(40000,90000,false,16.);
        assert(!exact.active && last.type==seek_step::finish && last.speed==2.f && !last.toggle_pause);
        seek_controller stuck;stuck.start(40000,10000,false,1.f);
        int frames=0;while(stuck.active && frames<1000){stuck.frame(10000,90000,false,16.);++frames;}
        assert(!stuck.active && frames<200);
        seek_controller end;end.start(95000,10000,false,1.f);
        for(int i=0;i<10 && end.active;++i)last=end.frame(89999,90000,false,16.);
        assert(!end.active && last.type==seek_step::finish);
    }
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
    auto quarter=sample_lens(path,125);assert(quarter.fov>60 && quarter.fov<65);
    std::vector<marker> rotating={{0,{},{},{170,30,1,1}},{1000,{},{},{-175,45,2,2}},
                                  {2500,{},{},{-140,65,4,4}}};
    auto rb=sample_lens(rotating,999),ra=sample_lens(rotating,1000),rf=sample_lens(rotating,1001);
    assert(std::abs((ra.roll-rb.roll)-(rf.roll-ra.roll))<.0001f);
    assert(std::abs((ra.fov-rb.fov)-(rf.fov-ra.fov))<.0001);
    for(int tick=0;tick<=2500;++tick){auto v=sample_lens(rotating,tick);assert(v.roll>=169.99f && v.roll<=220.01f && v.fov>=29.99 && v.fov<=65.01);}
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
