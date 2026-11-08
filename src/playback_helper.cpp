#include "playback_helper.hpp"
#include "playback_helper_policy.hpp"
#include "build_profile.hpp"
#include "native_calls.hpp"
#include <Windows.h>
#include <MH/MinHook.h>
#include <mutex>
#include <cstdio>
#include <atomic>

namespace theater::playback_helper {
namespace {
using begin_fn=void(*)(int,int);
using execute_fn=void(*)(const void*);
using load_fn=void(*)();
using error_fn=void(*)(int);
begin_fn original_begin=nullptr;
execute_fn original_execute=nullptr;
load_fn load_scripts=nullptr;
error_fn original_error=nullptr;
std::atomic<unsigned> errors_reported=0;
uintptr_t game=0,host=0;
logger output=nullptr;
std::mutex state_mutex;
snapshot state;
thread_local detail::pass_context pass;
thread_local unsigned forwarded=0,skipped=0;

bool copy(uintptr_t address,void* destination,size_t size) {
    SIZE_T got=0;
    return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),destination,size,&got) && got==size;
}
void report(const char* message) {
    {std::lock_guard lock(state_mutex);state.message=message;}
    if(output)output(message);
}
void log_script_error(int instance) noexcept {
    if(instance!=1 || errors_reported.fetch_add(1)>=8)return;
    try {
        detail::script_error error;
        if(!detail::read_error(copy,game,instance,error))return;
        char message[1200];
        std::snprintf(message,sizeof(message),"Client script error: terminal=%d code=%p message=%.1023s",int(error.terminal),reinterpret_cast<void*>(error.code),error.message.data());
        report(message);
    } catch(...) {} // Diagnostics must never change the VM's error path.
}
void script_error(int instance) {
    log_script_error(instance);
    // The VM may longjmp here. All diagnostic locks/objects were destroyed
    // before forwarding, and no native_calls TLS scope is left to unwind.
    original_error(instance);
}
bool helper_linked() {
    // A bounded check at client-script initialization, never a frame/timer scan.
    // This is the supported build's client objFileInfo table (500 x 0x50).
    uint32_t count=0;
    if(!copy(game+0x50efb60+4,&count,sizeof(count)) || count>500)return false;
    for(uint32_t index=0;index<count;++index) {
        uintptr_t object=0;
        if(!copy(game+0x50dc2e0+500*0x50+index*0x50,&object,sizeof(object)) || !object)continue;
        uint32_t offset=0;
        if(!copy(object+0x34,&offset,sizeof(offset)) || offset<0x48 || offset>0x400000)continue;
        std::array<char,256> name{};SIZE_T bytes=0;
        ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(object+offset),name.data(),name.size(),&bytes);
        auto end=static_cast<const char*>(std::memchr(name.data(),0,bytes));
        if(end && detail::classify(std::string_view(name.data(),size_t(end-name.data())))==detail::script_kind::helper)return true;
    }
    return false;
}
void execute_script(const void* name) {
    if(pass.depth) {
        if(!detail::forward_script(true,detail::foreign_script(copy,reinterpret_cast<uintptr_t>(name)))) {++skipped;return;}
        ++forwarded;
    }
    original_execute(name);
}
void begin_scripts(int instance,int user) {
    if(instance==1)errors_reported=0;
    const bool playback=reinterpret_cast<bool(*)()>(game+profile::demo_playing.rva)();
    const bool nested=pass.depth!=0;
    detail::begin(instance,playback,pass,
        [&]{native_calls::invoke(original_begin,instance,user);},
        [&]{
            forwarded=0;skipped=0;
            bool complete=detail::guarded_load([]{load_scripts();},[&](const char* error){
                char message[320];
                std::snprintf(message,sizeof(message),"Playback helper: client begin inst=%d demo=%d forwarded=%u skipped=%u extra loader failed: %.160s",instance,int(playback),forwarded,skipped,error);
                report(message);
            });
            if(!complete) {
                std::lock_guard lock(state_mutex);++state.loads;state.skipped=skipped;state.linked=false;return;
            }
            const bool linked=helper_linked();
            {std::lock_guard lock(state_mutex);++state.loads;state.skipped=skipped;state.linked=linked;}
            char message[256];
            std::snprintf(message,sizeof(message),"Playback helper: client begin inst=%d demo=%d forwarded=%u skipped=%u linked=%d",instance,int(playback),forwarded,skipped,int(linked));
            report(message);
        });
    if(instance==1 && (!playback || nested)) {
        char message[160];
        std::snprintf(message,sizeof(message),"Playback helper: client begin inst=%d demo=%d extra pass skipped (%s)",instance,int(playback),nested?"nested load":"not a film");
        report(message);
    }
}
void clear_hooks() {
    if(game && original_error) {MH_DisableHook(reinterpret_cast<void*>(game+detail::error_rva));MH_RemoveHook(reinterpret_cast<void*>(game+detail::error_rva));}
    if(game && original_begin) {MH_DisableHook(reinterpret_cast<void*>(game+detail::native_begin_rva));MH_RemoveHook(reinterpret_cast<void*>(game+detail::native_begin_rva));}
    if(host && original_execute) {MH_DisableHook(reinterpret_cast<void*>(host+detail::execute_rva));MH_RemoveHook(reinterpret_cast<void*>(host+detail::execute_rva));}
    original_begin=nullptr;original_execute=nullptr;load_scripts=nullptr;original_error=nullptr;
}
}
bool initialize(uintptr_t game_base,logger log) {
    if(current().hooked)return true;
    game=game_base;output=log;
    host=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"boiii.exe"));
    if(!game || !host || game==host || !detail::supported(copy,host,game)) {
        report("Playback helper: unsupported script-loader fingerprint; original loader left unchanged");return false;
    }
    std::array<unsigned char,16> demo_prefix{};
    if(!copy(game+profile::demo_playing.rva,demo_prefix.data(),demo_prefix.size()) ||
        std::memcmp(demo_prefix.data(),profile::demo_playing.bytes,demo_prefix.size())) {
        report("Playback helper: unsupported demo guard; original loader left unchanged");return false;
    }
    auto initialized=MH_Initialize();
    if(initialized!=MH_OK && initialized!=MH_ERROR_ALREADY_INITIALIZED) {
        report("Playback helper: hook runtime unavailable; original loader left unchanged");return false;
    }
    if(MH_CreateHook(reinterpret_cast<void*>(host+detail::execute_rva),execute_script,reinterpret_cast<void**>(&original_execute))!=MH_OK ||
        MH_CreateHook(reinterpret_cast<void*>(game+detail::native_begin_rva),begin_scripts,reinterpret_cast<void**>(&original_begin))!=MH_OK) {
        clear_hooks();report("Playback helper: could not prepare client-loader hooks; original loader left unchanged");return false;
    }
    load_scripts=reinterpret_cast<load_fn>(host+detail::loader_rva);
    if(MH_EnableHook(reinterpret_cast<void*>(host+detail::execute_rva))!=MH_OK ||
        MH_EnableHook(reinterpret_cast<void*>(game+detail::native_begin_rva))!=MH_OK) {
        clear_hooks();report("Playback helper: could not enable client-loader hooks; original loader left unchanged");return false;
    }
    {std::lock_guard lock(state_mutex);state.hooked=true;state.linked=false;}
    if(detail::match(copy,game+detail::error_rva,detail::error_prefix) &&
        detail::match(copy,game+detail::error_fields_rva,detail::error_fields) &&
        MH_CreateHook(reinterpret_cast<void*>(game+detail::error_rva),script_error,reinterpret_cast<void**>(&original_error))==MH_OK &&
        MH_EnableHook(reinterpret_cast<void*>(game+detail::error_rva))==MH_OK) {
        report("Playback helper: verified client script error logging enabled (event only)");
    } else {
        if(original_error) {MH_RemoveHook(reinterpret_cast<void*>(game+detail::error_rva));original_error=nullptr;}
        report("Playback helper: script error logger unavailable; original error handling unchanged");
    }
    report("Playback helper: verified client-film loader installed; waiting for the next film");return true;
}
void shutdown() {
    clear_hooks();
    std::lock_guard lock(state_mutex);state.hooked=false;state.linked=false;
}
snapshot current() {std::lock_guard lock(state_mutex);return state;}
}
