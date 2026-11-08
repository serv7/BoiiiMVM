#include "../src/animations.hpp"
#include "../src/animation_records.hpp"
#include "../src/death_catalog.hpp"
#include <Windows.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
namespace theater { void log_message(const std::string&){} }
namespace native_calls {thread_local void* target;void** target_slot(){return &target;}uintptr_t proxy_address(){return uintptr_t(target);}}
std::vector<void*> assets;
void enumerate(int,void(*)(void*,void*),void*,bool){assert(false && "Animation discovery must not call the blocking engine enumerator");}
int main(){
    using namespace theater::animations;
    auto base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x20000000,MEM_RESERVE,PAGE_READWRITE));assert(base);
    auto commit=[&](size_t offset,size_t bytes){size_t first=offset&~size_t(4095),last=(offset+bytes+4095)&~size_t(4095);assert(VirtualAlloc(base+first,last-first,MEM_COMMIT,PAGE_READWRITE));};
    commit(0x1420970,64);commit(0x94073f0+3*32,32);commit(0x16dd3540,128);commit(0x16cf4930+0x128,0x128);commit(0x17e77110+0x58,0x58);
    auto fn=base+0x1420970;unsigned char prefix[]={0x48,0x8b,0xc4,0x89,0x48,8,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    std::memcpy(fn,prefix,sizeof(prefix));unsigned char suffix[]={0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41,0x5c,0x5f,0x48,0xb8};
    std::memcpy(fn+15,suffix,sizeof(suffix));auto destination=uintptr_t(enumerate);std::memcpy(fn+26,&destination,8);fn[34]=0xff;fn[35]=0xe0;
    DWORD previous;assert(VirtualProtect(base+0x1420000,4096,PAGE_EXECUTE_READ,&previous));
    *reinterpret_cast<unsigned*>(base+0x94073f0+3*32+8)=record_size;
    std::vector<record> pool(25200);auto& first=pool[0];auto& second=pool[25199];
    *reinterpret_cast<uintptr_t*>(base+0x94073f0+3*32)=uintptr_t(pool.data());
    *reinterpret_cast<unsigned*>(base+0x94073f0+3*32+12)=unsigned(pool.size());
    const char* target="pb_death_run_forward_crumple";const char* source="pb_death_run_stumble";
    auto setup=[](record& value,const char* name,float fps){std::memcpy(value.data()+name_offset,&name,8);uint16_t bones=80,frames=100;std::memcpy(value.data()+bones_offset,&bones,2);std::memcpy(value.data()+frames_offset,&frames,2);std::memcpy(value.data()+fps_offset,&fps,4);};
    for(size_t i=2;i<20000;++i)setup(pool[i],"pb_rifle_walk_forward",30);
    setup(pool[3],"ai_face_male_death_1",30);setup(pool[4],"ai_death_run",30);
    setup(first,target,30);setup(second,source,60);auto original=first;assets={first.data(),second.data()};
    struct tree_type{uintptr_t anims;uint32_t child,client;} tree{0,1,0};
    *reinterpret_cast<uint16_t*>(base+0x16dd3540+18*2)=1;
    *reinterpret_cast<uintptr_t*>(base+0x16cf4930+0x128)=uintptr_t(&tree);
    auto info=base+0x17e77110+0x58;*reinterpret_cast<uintptr_t*>(info+8)=uintptr_t(first.data());*reinterpret_cast<float*>(info+0x1c)=1;*reinterpret_cast<float*>(info+0x10)=.4f;
    // A free-list slot can still contain stale animation payload. Never list it.
    pool[1]=second;auto free_next=uintptr_t(pool[2].data());std::memcpy(pool[1].data(),&free_next,8);
    assert(initialize(uintptr_t(base)));begin();auto started=std::chrono::steady_clock::now();
    // Opening a film/panel and ordinary game updates must not start any scan.
    auto instant=current();assert(instant.ready && instant.catalog && instant.catalog->size()==death_catalog.size());
    auto wait_catalog=[&]{auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(current().loading && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(current().ready && !current().loading);};
    auto state=current();
    assert(state.ready && state.entries.size()==162 && state.catalog && state.catalog->size()==162);
    assert(state.active.empty() && state.active_scans==0 && state.address_scans==0 && !state.loading);
    for(int i=0;i<1000;++i){update(i%2==0);current();}
    assert(current().address_scans==0 && current().active_scans==0);
    enqueue({operation::inspect_active,{},{},1234});update(false);assert(current().active_scans==0);
    assert(current().address_scans==0);
    enqueue({operation::inspect_active,{},{},1234});update(true);wait_catalog();update(true);state=current();
    assert(state.active.size()==1 && state.active[0].name==target && state.active[0].entity==18 && state.active_scans==1 && state.active_tick==1234);
    for(int i=0;i<1000;++i){update(i%2==0);current();}
    assert(current().active_scans==1 && current().address_scans==1 && "Opening the panel, playback and speed changes must not scan again");
    // Cached results survive movement/playback. Repeated queued clicks coalesce
    // to exactly one new snapshot, reusing resolved addresses.
    for(int i=0;i<10;++i)enqueue({operation::inspect_active,{},{},2000+i});
    update(true);assert(current().active_scans==2 && current().active_tick==2009 && current().address_scans==1);
    std::cout<<"25,200-slot pool with 20,000 unrelated records resolved once to 2 MP deaths in "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count()<<" ms after a manual scan.\n";
    enqueue({operation::replace,target,source});update(false);assert(first==original);
    enqueue({operation::replace,target,"pb_crouch_death_clutchchest"});update(true);assert(first==original);
    assert(current().message=="One of these death animations is not available in this film.");
    enqueue({operation::replace,target,source});update(true);assert(first!=original && *reinterpret_cast<const char**>(first.data())==target);
    enqueue({operation::restore_all});update(true);assert(first==original);
    enqueue({operation::replace,target,source});update(true);end();assert(first==original);
    for(int i=0;i<8;++i){begin();enqueue({operation::inspect_active});update(true);end();} // Cancel stale jobs across films.
    begin();assert(current().entries.size()==162 && !current().loading && current().address_scans==0);
    *reinterpret_cast<unsigned*>(base+0x94073f0+3*32+12)=0;
    enqueue({operation::inspect_active});update(true);wait_catalog();update(true);
    for(int i=0;i<1000;++i){update(true);current();}
    assert(current().address_scans==1 && current().active_scans==1);
    *reinterpret_cast<unsigned*>(base+0x94073f0+3*32+12)=unsigned(pool.size());
    enqueue({operation::inspect_active});update(true);wait_catalog();update(true);
    assert(current().address_scans==2 && current().active_scans==2 && current().active.size()==1);
    shutdown();
    VirtualFree(base,0,MEM_RELEASE);
    std::cout<<"Animation runtime checks passed: fixed 162-name catalog available on the first UI read, asynchronous MP-only address checks without engine calls or game updates, refresh cancellation, free-slot exclusion, active corpse tree, paused swap guard, source payload, name preservation and restoration.\n";
}
