#include "animations.hpp"
#include "animation_records.hpp"
#include "death_catalog.hpp"
#include "backend.hpp"
#include <Windows.h>
#include <deque>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <set>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string_view>
namespace theater::animations {
namespace {
uintptr_t game=0;bool initialized=false,active=false;
std::mutex guard;snapshot state;std::deque<request> pending;swaps controller;
std::map<std::string,uintptr_t> loaded;std::map<uintptr_t,std::string> names;
bool addresses_resolved=false;
struct discovery {uint64_t generation=0;uintptr_t pool=0;uint32_t count=0;std::map<std::string,uintptr_t> clips;std::string error;long long milliseconds=0;};
std::mutex scan_guard;std::condition_variable scan_wake;std::thread scan_thread;
std::atomic<uint64_t> scan_generation=0;std::atomic<bool> scan_stopping=false;
bool scan_requested=false,scanning=false;uint64_t requested_generation=0;uintptr_t requested_game=0;
std::shared_ptr<discovery> completed;
constexpr uintptr_t enumerate_assets=0x1420970,asset_pools=0x94073f0;
constexpr unsigned char enum_prefix[]={0x48,0x8b,0xc4,0x89,0x48,0x08,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
bool accessible(uintptr_t p,size_t size,bool write=false) {
    MEMORY_BASIC_INFORMATION info{};
    return p && VirtualQuery(reinterpret_cast<void*>(p),&info,sizeof(info)) && info.State==MEM_COMMIT &&
        !(info.Protect&(PAGE_NOACCESS|PAGE_GUARD)) && p+size<=uintptr_t(info.BaseAddress)+info.RegionSize &&
        (!write || (info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)));
}
std::string name_at(uintptr_t p) {
    MEMORY_BASIC_INFORMATION info{};
    if(!p || !VirtualQuery(reinterpret_cast<void*>(p),&info,sizeof(info)) || info.State!=MEM_COMMIT || (info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return {};
    size_t limit=std::min<size_t>(256,uintptr_t(info.BaseAddress)+info.RegionSize-p);
    const char* text=reinterpret_cast<const char*>(p);size_t length=0;
    for(;length<limit && text[length];++length)if(text[length]<32 || text[length]>126)return {};
    return length<limit?std::string(text,length):std::string{};
}
struct adapter {
    uintptr_t find(const std::string& name){auto it=loaded.find(name);return it==loaded.end()?0:it->second;}
    bool read(const std::string& name,uintptr_t address,record& out) {
        if(!accessible(address,out.size()))return false;
        std::memcpy(out.data(),reinterpret_cast<void*>(address),out.size());
        uintptr_t text;std::memcpy(&text,out.data()+name_offset,8);
        uint16_t bones,frames;std::memcpy(&bones,out.data()+bones_offset,2);std::memcpy(&frames,out.data()+frames_offset,2);
        float fps;std::memcpy(&fps,out.data()+fps_offset,4);
        return name_at(text)==name && frames>0 && bones>0 && bones<=1024 && std::isfinite(fps) && fps>0 && fps<=1000;
    }
    bool write(uintptr_t address,const record& value){if(!accessible(address,value.size(),true))return false;std::memcpy(reinterpret_cast<void*>(address),value.data(),value.size());return true;}
} memory;
bool mp_death(std::string_view name){return name.starts_with("pb_") && name.find("death")!=std::string_view::npos;}
bool copy_memory(uintptr_t address,void* destination,size_t bytes) {
    SIZE_T copied=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),destination,bytes,&copied) && copied==bytes;
}
std::shared_ptr<discovery> discover(uintptr_t base,uint64_t generation) {
    auto result=std::make_shared<discovery>();result->generation=generation;
    auto started=std::chrono::steady_clock::now();unsigned char header[32]{};
    if(!copy_memory(base+asset_pools+3*32,header,sizeof(header))){result->error="Animation pool is unavailable.";return result;}
    uint32_t stride;std::memcpy(&result->pool,header,8);std::memcpy(&stride,header+8,4);std::memcpy(&result->count,header+12,4);
    if(stride!=record_size || !result->pool || !result->count || result->count>100000){result->error="Unsupported animation pool layout.";return result;}
    // One bounded snapshot avoids thousands of VirtualQuery calls and does not
    // depend on paused/scaled game-update frequency. Only MP body death assets
    // are retained. ReadProcessMemory safely refuses memory unloaded meanwhile.
    std::vector<record> records(result->count);
    if(!copy_memory(result->pool,records.data(),records.size()*record_size)){result->error="Animation assets changed while loading. Refresh the list.";return result;}
    for(size_t i=0;i<records.size();++i) {
        if((i&127)==0 && (scan_stopping || scan_generation!=generation))return {};
        const auto& value=records[i];uintptr_t text;std::memcpy(&text,value.data()+name_offset,8);
        if(!text || (text>=result->pool && text<result->pool+records.size()*record_size))continue;
        char name[256]{};SIZE_T bytes=0;
        ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(text),name,sizeof(name),&bytes);
        if(bytes<4 || std::memcmp(name,"pb_",3))continue;
        const char* end=static_cast<const char*>(std::memchr(name,0,bytes));if(!end)continue;
        std::string_view candidate(name,size_t(end-name));if(!mp_death(candidate))continue;
        bool valid=true;for(unsigned char c:candidate)if(c<32 || c>126){valid=false;break;}
        uint16_t bones,frames;float fps;
        std::memcpy(&bones,value.data()+bones_offset,2);std::memcpy(&frames,value.data()+frames_offset,2);std::memcpy(&fps,value.data()+fps_offset,4);
        if(valid && bones>0 && bones<=1024 && frames>0 && std::isfinite(fps) && fps>0 && fps<=1000)
            result->clips.emplace(std::string(candidate),result->pool+i*record_size);
    }
    unsigned char after[32]{};
    if(!copy_memory(base+asset_pools+3*32,after,sizeof(after)) || std::memcmp(header,after,16)){
        result->clips.clear();result->error="Animation assets changed while loading. Refresh the list.";
    }
    result->milliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
    return result;
}
void discovery_worker() {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    for(;;) {
        uint64_t generation;uintptr_t base;
        {std::unique_lock lock(scan_guard);scan_wake.wait(lock,[]{return scan_stopping || scan_requested;});
            if(scan_stopping)return;generation=requested_generation;base=requested_game;scan_requested=false;}
        std::shared_ptr<discovery> result;
        try{result=discover(base,generation);}catch(...){result=std::make_shared<discovery>();result->generation=generation;result->error="Could not read multiplayer death animations.";}
        std::lock_guard lock(scan_guard);if(result && generation==scan_generation)completed=std::move(result);
    }
}
void start_scan() {
    if(!initialized)return;
    ++state.address_scans;
    scanning=true;state.loading=true;
    auto generation=++scan_generation;
    {std::lock_guard lock(scan_guard);completed.reset();requested_generation=generation;requested_game=game;scan_requested=true;}
    if(!scan_thread.joinable())scan_thread=std::thread(discovery_worker);
    scan_wake.notify_one();
}
void publish() {
    // The list is a fixed catalog, as in T6MVM. Runtime address discovery must
    // never gate displaying, searching, selecting, or previewing these names.
    std::map<std::string,entry> catalog;
    for(auto name:death_catalog){std::string key(name);catalog.emplace(key,entry{key,controller.replacement(key)});}
    names.clear();
    for(const auto& [name,address]:loaded){
        catalog[name]={name,controller.replacement(name)};names[address]=name;
    }
    state.entries.clear();auto all=std::make_shared<std::vector<entry>>();all->reserve(catalog.size());
    for(auto& [name,clip]:catalog)all->push_back(std::move(clip));
    state.entries=*all;state.catalog=std::move(all);state.ready=initialized && active;
}
void poll_scan() {
    std::shared_ptr<discovery> result;
    {std::lock_guard lock(scan_guard);result=std::move(completed);}
    if(!result || result->generation!=scan_generation || !active)return;
    scanning=false;state.loading=false;
    addresses_resolved=true;
    if(!result->error.empty()){state.message=result->error;return;}
    loaded=std::move(result->clips);publish();
    state.message=std::to_string(state.entries.size())+" multiplayer death animations ("+std::to_string(loaded.size())+" available in this film).";
    log_message("Animations: "+state.message+" Discovery took "+std::to_string(result->milliseconds)+" ms on the background worker.");
}
void pump_scan() {
    poll_scan();
}
void scan_active() {
    state.active.clear();
    // Retail DObj handle map and XAnimInfo layout verified from GetAnimTime's
    // native traversal and the paused film, including the two active victims.
    constexpr uintptr_t handles=0x16dd3540,objects=0x16cf4930,infos=0x17e77110;
    if(!accessible(game+handles,64*2))return;
    for(int entity=0;entity<64;++entity) {
        unsigned index=*reinterpret_cast<uint16_t*>(game+handles+entity*2);
        if(!index || index>=0xc00)continue;
        auto object=game+objects+index*0x128;if(!accessible(object,8))continue;
        auto tree=*reinterpret_cast<uintptr_t*>(object);if(!accessible(tree,16))continue;
        uint32_t root=*reinterpret_cast<uint32_t*>(tree+8);
        std::vector<uint32_t> stack={root};std::set<uint32_t> seen;
        while(!stack.empty() && seen.size()<1024) {
            auto node=stack.back();stack.pop_back();if(!node || node>=65536 || !seen.insert(node).second)continue;
            auto info=game+infos+node*0x58;if(!accessible(info,0x58))continue;
            stack.push_back(*reinterpret_cast<uint32_t*>(info+0x44));stack.push_back(*reinterpret_cast<uint32_t*>(info+0x48));
            auto parts=*reinterpret_cast<uintptr_t*>(info+8);auto it=names.find(parts);
            if(it==names.end() && loaded.empty() && accessible(parts,record_size)) {
                auto name=name_at(*reinterpret_cast<uintptr_t*>(parts+name_offset));record value;
                if(mp_death(name) && memory.read(name,parts,value))it=names.emplace(parts,std::move(name)).first;
            }
            if(it==names.end())continue;
            float weight=*reinterpret_cast<float*>(info+0x1c),time=*reinterpret_cast<float*>(info+0x10);
            if(std::isfinite(weight) && weight>.0001f && std::isfinite(time))state.active.push_back({it->second,entity,time,weight});
        }
    }
}
}
bool initialize(uintptr_t base) {
    std::lock_guard lock(guard);game=base;
    initialized=accessible(base+enumerate_assets,sizeof(enum_prefix)) && !std::memcmp(reinterpret_cast<void*>(base+enumerate_assets),enum_prefix,sizeof(enum_prefix));
    log_message(initialized?"Animation database enumerator verified":"Animation database fingerprint unavailable");return initialized;
}
void begin(){std::lock_guard lock(guard);++scan_generation;active=true;pending.clear();controller.clear();loaded.clear();names.clear();scanning=false;addresses_resolved=false;state={};publish();state.message="Pause during a death, then press Scan active deaths. No automatic scanning.";}
void update(bool paused) {
    std::lock_guard lock(guard);if(!active)return;
    pump_scan();
    auto work=std::move(pending);pending.clear();
    bool changed=false;
    for(const auto& request:work) {
        if(request.type==operation::inspect_active){
            if(!state.ready){state.message="Load a film before scanning active deaths.";continue;}
            if(!paused){state.message="Pause the demo before scanning active deaths.";continue;}
            if(!addresses_resolved && !scanning)start_scan();
            if(scanning){pending.push_back(request);continue;}
            auto started=std::chrono::steady_clock::now();scan_active();
            ++state.active_scans;state.active_tick=request.tick;
            state.active_scan_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
            state.message="Scan complete: "+std::to_string(state.active.size())+" active death clips. Scanning stopped.";
            continue;
        }
        if(request.type==operation::refresh){start_scan();continue;}
        if(!state.ready){state.message="Load a film before changing animation swaps.";continue;}
        if(!paused){state.message="Pause the demo before changing animation swaps.";continue;}
        if(!addresses_resolved && !scanning)start_scan();
        if(scanning){pending.push_back(request);continue;}
        if(request.type==operation::replace && (!loaded.contains(request.target) || !loaded.contains(request.source))){state.message="One of these death animations is not available in this film.";continue;}
        bool okay=request.type==operation::replace?controller.replace(request.target,request.source,memory):
            request.type==operation::restore?controller.restore(request.target,memory):controller.restore_all(memory);
        state.message=okay?"Applied. Rewind before the death and replay it.":"Swap refused: choose loaded, different clips; an asset may have changed.";
        log_message("Animations: "+state.message);
        changed=true;
    }
    if(changed)publish();
}
void end() {
    std::lock_guard lock(guard);++scan_generation;if(active && controller.has_changes())controller.restore_all(memory);active=false;scanning=false;addresses_resolved=false;controller.clear();loaded.clear();names.clear();pending.clear();state={};
}
void shutdown(){end();scan_stopping=true;scan_wake.notify_one();if(scan_thread.joinable())scan_thread.join();scan_stopping=false;std::lock_guard lock(scan_guard);scan_requested=false;completed.reset();}
snapshot current(){std::lock_guard lock(guard);pump_scan();return state;}
void enqueue(request value){
    std::lock_guard lock(guard);
    if(pending.size()>=64)return;
    if(value.type==operation::inspect_active){
        // A failed/empty address lookup can be retried by another click, never
        // by a timer. Requests waiting for the same pass coalesce.
        if(active && loaded.empty() && !scanning)addresses_resolved=false;
        for(auto& queued:pending)if(queued.type==operation::inspect_active){queued=std::move(value);return;}
    }
    pending.push_back(std::move(value));
}
}
