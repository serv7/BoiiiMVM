#include "../src/editor.hpp"
#include "../src/backend.hpp"
#include <Windows.h>
#include <MH/MinHook.h>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
namespace theater {
void log_message(const std::string&){}
}
namespace theater::editor {
config controls;
config capture_controls(scene value) {auto copy=controls;copy.values=std::move(value);return copy;}
void apply_controls(const config& value) {controls=value;}
vec3 destination;
void goto_position(vec3 value) {destination=value;}
}
int main() {
    using namespace theater::editor;
    auto game=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x20000000,MEM_RESERVE,PAGE_READWRITE));assert(game);
    auto commit=[&](size_t address,size_t size) {size_t start=address&~size_t(4095),end=(address+size+4095)&~size_t(4095);assert(VirtualAlloc(game+start,end-start,MEM_COMMIT,PAGE_READWRITE));};
    commit(0x1ca82f0,0x1000);commit(3798102,23);commit(80930164,36);commit(397173196,84+160*13);
    // Execute the verified fog-copy ABI with an equivalent 144-byte copy body.
    auto code=game+0x1ca82f0;size_t n=0;
    for(unsigned offset=0;offset<128;offset+=4) {
        if(offset==0) {code[n++]=0x8b;code[n++]=2;code[n++]=0x89;code[n++]=1;}
        else {code[n++]=0x8b;code[n++]=0x42;code[n++]=offset;code[n++]=0x89;code[n++]=0x41;code[n++]=offset;}
    }
    for(unsigned offset=128;offset<144;offset+=4) {code[n++]=0x8b;code[n++]=0x82;std::memcpy(code+n,&offset,4);n+=4;code[n++]=0x89;code[n++]=0x81;std::memcpy(code+n,&offset,4);n+=4;}
    code[n++]=0x48;code[n++]=0x8b;code[n++]=0xc1;code[n++]=0xc3;
    unsigned char blood[]={136,132,26,160,100,19,0,68,136,188,26,162,100,19,0,68,136,164,26,161,100,19,0};std::memcpy(game+3798102,blood,23);
    sun original_sun;original_sun.brightness=73;original_sun.tint={0,0,0};
    std::memcpy(game+80930164,&original_sun,36);
    *reinterpret_cast<int*>(game+397173196)=13;
    const char* names[]={"cg_gun_x","cg_gun_y","cg_gun_z","r_exposureValue","r_exposureTweak","r_skyRotation","r_skyTransition","cg_drawGun","cg_draw2d","cg_drawCrosshair","r_DedicatedPlayerShadowCull","r_DedicatedPlayerSunShadowResolution","demo_dollycamHighlightThreshholdDistance"};
    for(int i=0;i<13;++i) {
        auto p=game+397173280+i*160;*reinterpret_cast<uint32_t*>(p)=hash_name(names[i]);
        bool boolean=i==4 || (i>=7 && i<=9);bool integer=i==10 || i==11;*reinterpret_cast<uint32_t*>(p+28)=boolean?1:integer?(i==11?7:6):2;
        if(boolean)p[40]=1;
        else if(integer)*reinterpret_cast<int32_t*>(p+40)=i==10?2:1024;
        else *reinterpret_cast<float*>(p+40)=i==3?10.f:i==12?6.5f:0.f;
        *reinterpret_cast<uint64_t*>(p+64)=boolean?encode_bool(true,uint32_t(__readgsqword(0x60))):integer?encode_int(i==10?2:1024,uint32_t(__readgsqword(0x60))):encode_float(i==3?10.f:i==12?6.5f:0.f,uint32_t(__readgsqword(0x60)));
    }
    std::array<unsigned char,2080> dvar_baseline;std::memcpy(dvar_baseline.data(),game+397173280,dvar_baseline.size());
    auto session=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xa00000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(session);
    auto lights=reinterpret_cast<light*>(session+9979404);lights[0]={};lights[0].intensity=42;*reinterpret_cast<int*>(session+9981004)=1;
    std::array<unsigned char,144> fog_source{},fog_dest{};fog native_fog;native_fog.half_distance=1234;native_fog.padding.fill(0x5a);fog_source[0]=0x72;std::memcpy(fog_source.data()+20,&native_fog,124);
    DWORD old;assert(VirtualProtect(game+0x1ca8000,8192,PAGE_EXECUTE_READ,&old));
    std::filesystem::create_directories("build/editor-fixture");auto previous=std::filesystem::current_path();std::filesystem::current_path("build/editor-fixture");
    assert(MH_Initialize()==MH_OK);assert(initialize(uintptr_t(game)));begin(uintptr_t(session));
    auto copier=reinterpret_cast<void*(*)(void*,const void*)>(code);copier(fog_dest.data(),fog_source.data());update();
    auto s=current();assert(s.fog_ready && s.sun_ready && s.misc_ready && s.lights_ready && s.blood_ready && s.values.mist.half_distance==1234);
    assert(s.values.sunlight.tint.r==1.f && s.values.sunlight.tint.g>.9f && s.values.sunlight.tint.b>.8f);
    assert(s.useful_dvars_ready && !s.useful_dvars);
    enqueue({operation::toggle_useful_dvars,{},-1,"",1});update();
    assert(current().useful_dvars);
    for(int i=10;i<12;++i)assert(*reinterpret_cast<int32_t*>(game+397173280+i*160+40)==0 && *reinterpret_cast<uint64_t*>(game+397173280+i*160+64)==encode_int(0,uint32_t(__readgsqword(0x60))));
    assert(*reinterpret_cast<float*>(game+397173280+12*160+40)==3.f);
    enqueue({operation::toggle_useful_dvars,{},-1,"",0});update();
    assert(!current().useful_dvars && !std::memcmp(dvar_baseline.data(),game+397173280,dvar_baseline.size()));
    auto second_type=reinterpret_cast<uint32_t*>(game+397173280+11*160+28);
    *second_type=8;enqueue({operation::toggle_useful_dvars,{},-1,"",1});update();
    assert(!current().useful_dvars && !std::memcmp(dvar_baseline.data()+10*160,game+397173280+10*160,160));
    *second_type=7;
    enqueue({operation::toggle_useful_dvars,{},-1,"",1});update();assert(current().useful_dvars);
    s.values.fog_enabled=true;s.values.mist.half_distance=321;s.values.mist.padding.fill(0);s.values.sync_colors=true;s.values.mist.tint={.2f,.3f,.4f};
    enqueue({operation::change_fog,s.values});update();copier(fog_dest.data(),fog_source.data());
    fog rendered;std::memcpy(&rendered,fog_dest.data()+20,124);assert(rendered.half_distance==321 && rendered.padding==native_fog.padding && fog_dest[0]==0x72 && rendered.haze_tint.b==.4f);
    s=current();s.values.sunlight.brightness=999;enqueue({operation::change_sun,s.values});update();
    assert(reinterpret_cast<sun*>(game+80930164)->brightness==999);
    assert(reinterpret_cast<sun*>(game+80930164)->tint.r==1.f);
    s=current();s.values.sunlight.tint={0,0,0};enqueue({operation::change_sun,s.values,1});update();
    assert(reinterpret_cast<sun*>(game+80930164)->tint.r==0);
    enqueue({operation::reset_sun});update();
    assert(reinterpret_cast<sun*>(game+80930164)->tint.g>.9f);
    s=current();s.values.misc_enabled=true;s.values.exposure=7;s.values.gun.z=-4;s.values.remove_gun=true;s.values.remove_blood=true;s.values.remove_hud=true;
    enqueue({operation::change_misc,s.values});update();assert(game[3798102]==0x90 && session[0x68abbd]==1);
    assert(*reinterpret_cast<float*>(game+397173280+3*160+40)==7 && game[397173280+7*160+40]==0);
    enqueue({operation::add_light});update();assert(*reinterpret_cast<int*>(session+9981004)==2);
    s=current();s.values.lights[1].intensity=87;enqueue({operation::change_light,s.values,1});update();assert(lights[1].intensity==87);
    enqueue({operation::save_cfg,{},-1,"test.cfg"});update();assert(std::filesystem::exists("MVM/Theater/configs/test.cfg"));
    enqueue({operation::restore});update();assert(!current().useful_dvars && reinterpret_cast<sun*>(game+80930164)->brightness==73 && game[3798102]==blood[0] && session[0x68abbd]==0);
    assert(*reinterpret_cast<int*>(session+9981004)==1 && lights[0].intensity==42 && !std::memcmp(dvar_baseline.data(),game+397173280,dvar_baseline.size()));
    enqueue({operation::load_cfg,{},-1,"test.cfg"});update();assert(lights[1].intensity==87 && session[0x68abbd]==1);
    enqueue({operation::save_fog,{},-1,"test.zfog"});enqueue({operation::save_lights,{},-1,"test.bo3light"});update();assert(std::filesystem::exists("MVM/Theater/fog-presets/test.zfog") && std::filesystem::exists("MVM/Theater/lighting/test.bo3light"));
    enqueue({operation::goto_light,{},1});update();assert(destination.x==lights[1].position.x);
    enqueue({operation::toggle_useful_dvars,{},-1,"",1});update();assert(current().useful_dvars);
    end();assert(!std::memcmp(dvar_baseline.data(),game+397173280,dvar_baseline.size()) && game[3798102]==blood[0] && *reinterpret_cast<int*>(session+9981004)==1);
    copier(fog_dest.data(),fog_source.data());assert(!std::memcmp(fog_source.data(),fog_dest.data(),144));shutdown();MH_Uninitialize();
    std::filesystem::current_path(previous);VirtualFree(session,0,MEM_RELEASE);VirtualFree(game,0,MEM_RELEASE);
    std::cout<<"Editor runtime tests passed: native fog hook/header/padding, sun/dvars/blood/lights/useful-dvar toggle, actual CFG/preset/lighting IO and full restoration.\n";
}
