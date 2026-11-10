#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace theater {
inline double zoom_fov(double current,float notches,float step) {
    return std::clamp(current-double(notches)*step,.1,75.);
}
inline float timeline_fraction(int tick,int start,int end) {
    return end>start?std::clamp(float(double(tick)-start)/float(double(end)-start),0.f,1.f):0.f;
}
inline int timeline_tick(float fraction,int start,int end) {
    return end>start?int(start+std::clamp(fraction,0.f,1.f)*(double(end)-start)):start;
}
// Getting to an exact demo tick, the way T6MVM's DemoSeek does it in BO2. The engine's demo_back /
// demo_forward land on its own snapshot points, which can be short of the target (or barely move
// forward into parts of the film not played yet). So after the jump the film plays fast and slows
// down onto the target; if it ended up past the target it jumps back again (a few times at most).
struct seek_step {
    enum kind { none, jump, set_speed, finish } type=none;
    int tick=0;               // jump: target tick
    float speed=1;            // set_speed / finish: playback speed to set
    bool toggle_pause=false;  // send one demo_pause (it toggles)
};
class seek_controller {
    int settle=0,frames=0,stalled=0,jumps=0,last=0,unpause_wait=0;
    float restore=1;bool pause_at_end=false,unpause_sent=false;
public:
    // 10x is the game's own top speed; the jump gets close, so only the last few seconds are played.
    static constexpr float settle_speed=.05f,max_speed=10.f;
    static constexpr int settle_frames=3,max_jumps=3,max_frames=6000,max_stalled=120,overshoot=50,unpause_frames=30;
    bool active=false;int target=0;
    // Call when the jump command for target has just been sent.
    void start(int target_tick,int now,bool paused,float speed) {
        active=true;target=target_tick;restore=std::isfinite(speed) && speed>0?speed:1.f;pause_at_end=paused;
        settle=settle_frames;frames=0;stalled=0;jumps=0;last=now;unpause_sent=false;unpause_wait=0;
    }
    void cancel(){active=false;}
    seek_step finish(int now,bool paused) {
        active=false;
        // One toggle at most, and only if the state differs from how the user left it.
        return {seek_step::finish,now,restore,pause_at_end!=paused};
    }
    seek_step frame(int now,int end,bool paused,double frame_ms) {
        if(!active)return {};
        ++frames;
        if(settle>0) {
            // The tick follows a jump a frame or two late; creep meanwhile so the target isn't passed.
            if(--settle==0 && now>target+overshoot && jumps<max_jumps) {
                ++jumps;settle=settle_frames;
                return {seek_step::jump,target,settle_speed};
            }
            if(settle>0)return {seek_step::set_speed,0,settle_speed};
        }
        const int remaining=target-now;
        const bool film_over=end>0 && now>=end-1;
        if(remaining<=0 || frames>max_frames || film_over)return finish(now,paused);
        if(paused) {
            // The film has to run to get there. demo_pause toggles: send it once, never repeatedly
            // (dozens of toggles around a jump broke the first-person view). If it doesn't take, stay
            // where the jump landed.
            if(!unpause_sent){unpause_sent=true;unpause_wait=0;return {seek_step::set_speed,0,settle_speed,true};}
            if(++unpause_wait>unpause_frames)return finish(now,paused);
            return {seek_step::set_speed,0,settle_speed};
        }
        stalled=now>last?0:stalled+1;last=now;
        if(stalled>max_stalled)return finish(now,paused);
        // Cover about half the remaining time each frame: full speed far away, slowing onto the tick.
        const double ms=std::clamp(frame_ms,1.,100.);
        return {seek_step::set_speed,0,float(std::clamp(remaining*.5/ms,double(settle_speed),double(max_speed)))};
    }
};
inline constexpr std::array<float, 26> speeds = {
    .01f,.02f,.05f,.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f,
    1.f,1.25f,1.5f,1.75f,2.f,2.5f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f};
inline float step_speed(float current, int direction) {
    if (direction > 0) {
        for (float speed : speeds) if (speed > current + .0001f) return speed;
        return speeds.back();
    }
    for (auto it = speeds.rbegin(); it != speeds.rend(); ++it)
        if (*it < current - .0001f) return *it;
    return speeds.front();
}
struct vec3 { float x=0,y=0,z=0; };
inline float camera_speed(float value) {
    return std::isfinite(value)?std::clamp(value,.0001f,10.f):1.f;
}
inline float modified_camera_speed(float base,float fast,float slow,bool shift,bool control) {
    // Ctrl wins when both are held. These are multipliers of the user's base speed.
    return camera_speed(base)*(control?std::clamp(slow,.0001f,1.f):shift?std::clamp(fast,1.f,100.f):1.f);
}
inline int next_camera_mode(int current,int direction) {
    // Free roam is the entry state; subsequent clicks alternate Edit and Dolly.
    return current==2?1:current==1?2:direction>0?2:1;
}
class movement_scaler {
    bool initialized=false;
    vec3 written;
    double x=0,y=0,z=0;
public:
    void reset(){initialized=false;}
    vec3 apply(vec3 before,vec3 after,float scale) {
        // Keep fractional displacement so tiny steps survive float world-coordinate rounding.
        if(!initialized || before.x!=written.x || before.y!=written.y || before.z!=written.z) {
            x=before.x;y=before.y;z=before.z;initialized=true;
        }
        x+=(double(after.x)-before.x)*scale;
        y+=(double(after.y)-before.y)*scale;
        z+=(double(after.z)-before.z)*scale;
        written={float(x),float(y),float(z)};return written;
    }
};
inline vec3 operator-(vec3 a,vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline float dot(vec3 a,vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline float wrap_angle(float angle) {
    float result=std::fmod(angle+180.f,360.f);
    if(result<0) result+=360.f;
    return result-180.f;
}
inline float lerp_angle(float a,float b,float t) { return a+wrap_angle(b-a)*t; }
class demo_clock {
    bool initialized=false;int previous_tick=0;double value=0;
public:
    void reset(){initialized=false;}
    double sample(int tick,bool paused,double speed,double elapsed_ms) {
        double advance=std::clamp(elapsed_ms,0.,250.)*std::clamp(speed,0.,10.);
        if(!initialized || tick<previous_tick || std::abs(double(tick)-value)>std::max(100.,advance*4)){
            initialized=true;value=tick;
        }else if(paused){if(tick!=previous_tick)value=tick;}
        else value=std::clamp(value+advance,double(tick),double(tick)+.999999);
        previous_tick=tick;return value;
    }
};
struct lens { float roll=0; double fov=65; float focus=2,aperture=8; };
struct marker { int tick=0; vec3 position,direction; lens optics; };
inline float hermite(float a,float b,float va,float vb,float duration,float t) {
    const float t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*a+(t3-2*t2+t)*duration*va+
        (-2*t3+3*t2)*b+(t3-t2)*duration*vb;
}
inline std::optional<vec3> sample_dolly_position(const std::vector<marker>& cameras,double tick) {
    if(cameras.size()<2)return std::nullopt;
    for(size_t i=1;i<cameras.size();++i)
        if(cameras[i].tick<=cameras[i-1].tick)return std::nullopt;
    if(tick<=cameras.front().tick)return cameras.front().position;
    if(tick>=cameras.back().tick)return cameras.back().position;
    auto next=std::upper_bound(cameras.begin(),cameras.end(),tick,
        [](double value,const marker& camera){return value<camera.tick;});
    const size_t right=size_t(next-cameras.begin()),left=right-1;
    const float duration=float(cameras[right].tick-cameras[left].tick);
    const float t=float(tick-cameras[left].tick)/duration;
    // Interior velocity uses real marker times, so both neighbouring segments
    // meet with the same derivative. Zero end velocities give a gentle start/stop.
    auto position_velocity=[&](size_t index) {
        if(index==0 || index+1==cameras.size())return vec3{};
        vec3 incoming=cameras[index].position-cameras[index-1].position;
        vec3 outgoing=cameras[index+1].position-cameras[index].position;
        float left_time=float(cameras[index].tick-cameras[index-1].tick);
        float right_time=float(cameras[index+1].tick-cameras[index].tick);
        if(dot(incoming,outgoing)<0)return vec3{}; // reverse direction: stop at the marker
        float speed_limit=2.f*std::min(std::sqrt(dot(incoming,incoming))/left_time,
                                      std::sqrt(dot(outgoing,outgoing))/right_time);
        vec3 span=cameras[index+1].position-cameras[index-1].position;
        float factor=.8f/(left_time+right_time);
        vec3 velocity={span.x*factor,span.y*factor,span.z*factor};
        float speed=std::sqrt(dot(velocity,velocity));
        if(speed>speed_limit && speed>0) {
            factor=speed_limit/speed;
            velocity={velocity.x*factor,velocity.y*factor,velocity.z*factor};
        }
        return velocity;
    };
    vec3 left_velocity=position_velocity(left),right_velocity=position_velocity(right);
    auto position_axis=[&](int axis){
        auto coordinate=[&](size_t i){const auto& p=cameras[i].position;return axis==0?p.x:axis==1?p.y:p.z;};
        float va=axis==0?left_velocity.x:axis==1?left_velocity.y:left_velocity.z;
        float vb=axis==0?right_velocity.x:axis==1?right_velocity.y:right_velocity.z;
        return hermite(coordinate(left),coordinate(right),va,vb,duration,t);
    };
    vec3 result={position_axis(0),position_axis(1),position_axis(2)};
    if(!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z))return std::nullopt;
    return result;
}
template<class Value> inline double sample_lens_channel(const std::vector<marker>& markers,double tick,Value value,bool angular=false) {
    std::vector<double> values;values.reserve(markers.size());
    for(const auto& camera:markers) {
        double next=value(camera.optics);
        if(angular && !values.empty())next=values.back()+wrap_angle(float(next-values.back()));
        values.push_back(next);
    }
    if(tick<=markers.front().tick)return values.front();
    if(tick>=markers.back().tick)return values.back();
    auto next=std::upper_bound(markers.begin(),markers.end(),tick,[](double t,const marker& m){return t<m.tick;});
    size_t right=size_t(next-markers.begin()),left=right-1;
    auto velocity=[&](size_t index) {
        if(!index || index+1==markers.size())return 0.;
        double h0=double(markers[index].tick)-markers[index-1].tick,h1=double(markers[index+1].tick)-markers[index].tick;
        double d0=(values[index]-values[index-1])/h0,d1=(values[index+1]-values[index])/h1;
        if(d0*d1<=0)return 0.;
        // Shape-preserving time-weighted Hermite slopes are shared by both segments.
        double w0=2*h1+h0,w1=h1+2*h0;
        return (w0+w1)/(w0/d0+w1/d1);
    };
    double duration=double(markers[right].tick)-markers[left].tick,t=(tick-markers[left].tick)/duration;
    double t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*values[left]+(t3-2*t2+t)*duration*velocity(left)+
        (-2*t3+3*t2)*values[right]+(t3-t2)*duration*velocity(right);
}
inline lens sample_lens(const std::vector<marker>& markers,double tick) {
    if(markers.empty()) return {};
    for(size_t i=1;i<markers.size();++i)if(markers[i].tick<=markers[i-1].tick)return markers.front().optics;
    return {float(sample_lens_channel(markers,tick,[](lens v){return v.roll;},true)),
        sample_lens_channel(markers,tick,[](lens v){return v.fov;}),
        float(sample_lens_channel(markers,tick,[](lens v){return v.focus;})),
        float(sample_lens_channel(markers,tick,[](lens v){return v.aperture;}))};
}
inline std::optional<vec3> sample_dolly_angles(const std::vector<marker>& cameras,double tick) {
    if(cameras.size()<2)return std::nullopt;
    std::vector<marker> channels;channels.reserve(cameras.size());
    for(const auto& camera:cameras){
        // Retail add-marker stores position + forward * distance at +20:
        // this is a world-space look-at point, not Euler angles or a unit vector.
        auto forward=camera.direction-camera.position;double length=std::sqrt(dot(forward,forward));
        if(!std::isfinite(length) || length<.001)return std::nullopt;
        double horizontal=std::hypot(forward.x,forward.y);
        marker value;value.tick=camera.tick;
        value.optics.fov=std::atan2(-double(forward.z),horizontal)*57.29577951308232;
        value.optics.roll=horizontal<length*.000001 && !channels.empty()?channels.back().optics.roll:
            float(std::atan2(double(forward.y),double(forward.x))*57.29577951308232);
        channels.push_back(value);
    }
    for(size_t i=1;i<channels.size();++i)if(channels[i].tick<=channels[i-1].tick)return std::nullopt;
    return vec3{float(sample_lens_channel(channels,tick,[](lens v){return v.fov;})),
        float(sample_lens_channel(channels,tick,[](lens v){return v.roll;},true)),sample_lens(cameras,tick).roll};
}
// Keep marker selection close to the screen centre even at a very narrow FOV.
inline int hovered_marker(const std::vector<marker>& markers,vec3 eye,vec3 angles,
                          float fov_degrees=65.f) {
    constexpr float rad=.017453292519943295f;
    const float threshold_degrees=std::clamp(fov_degrees*.03f,.05f,2.f);
    const float cp=std::cos(angles.x*rad),sp=std::sin(angles.x*rad);
    const vec3 forward={cp*std::cos(angles.y*rad),cp*std::sin(angles.y*rad),-sp};
    float best=std::cos(threshold_degrees*rad),best_distance=std::numeric_limits<float>::max();
    int selected=-1;
    for(int i=0;i<int(markers.size());++i) {
        vec3 delta=markers[i].position-eye; float distance=std::sqrt(dot(delta,delta));
        if(distance<1.f) continue;
        float alignment=dot(delta,forward)/distance;
        if(alignment>best || (alignment>=best && distance<best_distance)) {
            selected=i; best=alignment; best_distance=distance;
        }
    }
    return selected;
}
}
