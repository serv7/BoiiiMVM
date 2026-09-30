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
struct lens { float roll=0; double fov=65; float focus=2,aperture=8; };
struct marker { int tick=0; vec3 position,direction; lens optics; };
inline float hermite(float a,float b,float va,float vb,float duration,float t) {
    const float t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*a+(t3-2*t2+t)*duration*va+
        (-2*t3+3*t2)*b+(t3-t2)*duration*vb;
}
inline std::optional<vec3> sample_dolly_position(const std::vector<marker>& cameras,int tick) {
    if(cameras.size()<2)return std::nullopt;
    for(size_t i=1;i<cameras.size();++i)
        if(cameras[i].tick<=cameras[i-1].tick)return std::nullopt;
    if(tick<=cameras.front().tick)return cameras.front().position;
    if(tick>=cameras.back().tick)return cameras.back().position;
    auto next=std::upper_bound(cameras.begin(),cameras.end(),tick,
        [](int value,const marker& camera){return value<camera.tick;});
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
inline lens sample_lens(const std::vector<marker>& markers,int tick) {
    if(markers.empty()) return {};
    if(tick<=markers.front().tick) return markers.front().optics;
    if(tick>=markers.back().tick) return markers.back().optics;
    auto next=std::upper_bound(markers.begin(),markers.end(),tick,
        [](int t,const marker& m){return t<m.tick;});
    const auto& b=*next; const auto& a=*(next-1);
    float t=float(tick-a.tick)/float(b.tick-a.tick);
    return {lerp_angle(a.optics.roll,b.optics.roll,t),
        a.optics.fov+(b.optics.fov-a.optics.fov)*t,
        a.optics.focus+(b.optics.focus-a.optics.focus)*t,
        a.optics.aperture+(b.optics.aperture-a.optics.aperture)*t};
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
