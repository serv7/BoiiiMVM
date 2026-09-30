#include "../src/native_calls.hpp"
#include "native_proxy_fixture.hpp"
#include "dvar_decoder_fixture.hpp"
#include "../src/editor.hpp"
#include <fstream>
#include <filesystem>
#include <bit>
#include <vector>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>

thread_local void* selected_target=nullptr;
thread_local std::vector<uintptr_t> returns;
uintptr_t proxy=0,placeholder=0;
extern "C" unsigned long _tls_index;
// A callback inside this module with the exact BOIII TLS-accessor layout.
#pragma section(".proxy",execute,read)
__declspec(allocate(".proxy")) const unsigned char getter_code[]={
    0x8b,0x05,0,0,0,0,0x65,0x48,0x8b,0x0c,0x25,0x58,0,0,0,
    0x48,0x8b,0x04,0xc1,0x48,0x8b,0x80,0,0,0,0,0xc3,0xcc,0xcc
};
void check_impl(bool condition,const char* expression,int line) {if(!condition) {std::fprintf(stderr,"FAILED at line %d: %s\n",line,expression);std::abort();}}
#define check(condition) check_impl((condition),#condition,__LINE__)
__declspec(noinline) void store_return(uintptr_t address) {returns.push_back(address);}
__declspec(noinline) uintptr_t pop_return() {check(!returns.empty());auto address=returns.back();returns.pop_back();return address;}
__declspec(noinline) int target(int a,int b,int c,int d) {
    check(reinterpret_cast<uintptr_t>(_ReturnAddress())==placeholder);
    return a+3*b+5*c+7*d;
}
__declspec(noinline) int nested(int value) {
    check(reinterpret_cast<uintptr_t>(_ReturnAddress())==placeholder);
    auto previous=selected_target;
    int result=native_calls::invoke(target,value,2,3,4);
    check(selected_target==previous && returns.size()==1);
    return result;
}
__declspec(noinline) void pointer_target(void* pointer,bool flag) {
    check(reinterpret_cast<uintptr_t>(_ReturnAddress())==placeholder);
    check(pointer==&selected_target && flag);
}
void write_pointer(unsigned char* data,size_t offset,uintptr_t value) {std::memcpy(data+offset,&value,8);}
int main() {
    std::setvbuf(stdout,nullptr,_IONBF,0);std::setvbuf(stderr,nullptr,_IONBF,0);
    // Execute the client's real proxy instruction layout in a separate process.
    // A native-style return stub pops the original caller from a TLS stack.
    auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(page!=nullptr);
    auto game=reinterpret_cast<uintptr_t>(page);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(page);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(page+0x80);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=65536;
    auto code=page+0x1200;proxy=reinterpret_cast<uintptr_t>(code);placeholder=game+0x1056;
    std::memcpy(code,proxy_fixture,sizeof(proxy_fixture));
    std::memcpy(page+0x4000,float_decoder,sizeof(float_decoder));
    std::memcpy(page+0x5000,bool_decoder,sizeof(bool_decoder));
    write_pointer(code,0x9b,placeholder);
    write_pointer(code,0xd0,reinterpret_cast<uintptr_t>(getter_code));
    write_pointer(code,0xd8,reinterpret_cast<uintptr_t>(store_return));
    page[0x1054]=0xff;page[0x1055]=0xcc;page[0x1056]=0x90;page[0x1057]=0xe9;
    int32_t jump=0x2000-0x105c;std::memcpy(page+0x1058,&jump,4);
    // callstack_return_stub shares the register-saving prologue, one aligned
    // callback, and the proxy's register-restoring epilogue.
    std::memcpy(page+0x2000,proxy_fixture,0x59);
    std::memcpy(page+0x2059,proxy_fixture+0xab,0x20);
    write_pointer(page+0x2000,0xd0,reinterpret_cast<uintptr_t>(pop_return));
    auto blocks=reinterpret_cast<uintptr_t*>(__readgsqword(0x58));
    auto offset=reinterpret_cast<uintptr_t>(&selected_target)-blocks[_tls_index];check(offset<=0x10000 && offset%8==0);
    auto displacement=reinterpret_cast<intptr_t>(&_tls_index)-reinterpret_cast<intptr_t>(getter_code+6);
    check(displacement>=INT32_MIN && displacement<=INT32_MAX);
    DWORD old;check(VirtualProtect(const_cast<unsigned char*>(getter_code),sizeof(getter_code),PAGE_EXECUTE_READWRITE,&old)!=0);
    int32_t disp=static_cast<int32_t>(displacement);std::memcpy(const_cast<unsigned char*>(getter_code)+2,&disp,4);
    uint32_t tls_offset=static_cast<uint32_t>(offset);std::memcpy(const_cast<unsigned char*>(getter_code)+22,&tls_offset,4);
    VirtualProtect(const_cast<unsigned char*>(getter_code),sizeof(getter_code),old,&old);
    check(VirtualProtect(page,65536,PAGE_EXECUTE_READ,&old)!=0);
    FlushInstructionCache(GetCurrentProcess(),nullptr,0);
    // Reproduce the client loader replacing PEB.ImageBaseAddress with the game.
    auto image_slot=reinterpret_cast<uintptr_t*>(__readgsqword(0x60)+0x10);
    auto original_image=*image_slot;*image_slot=game;
    check(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))==game);
    // Check the actual client's short displacement encoding as well as this
    // executable's larger TLS offset. No calls use the temporary short slot.
    check(VirtualProtect(const_cast<unsigned char*>(getter_code),sizeof(getter_code),PAGE_EXECUTE_READWRITE,&old)!=0);
    auto writable_getter=const_cast<unsigned char*>(getter_code);
    writable_getter[21]=0x40;writable_getter[22]=8;writable_getter[23]=0xc3;
    FlushInstructionCache(GetCurrentProcess(),writable_getter,sizeof(getter_code));
    check(native_calls::initialize(game) && native_calls::target_slot()==reinterpret_cast<void**>(blocks[_tls_index]+8));
    writable_getter[21]=0x80;std::memcpy(writable_getter+22,&tls_offset,4);writable_getter[26]=0xc3;
    VirtualProtect(writable_getter,sizeof(getter_code),old,&old);
    FlushInstructionCache(GetCurrentProcess(),writable_getter,sizeof(getter_code));
    bool initialized=native_calls::initialize(game);
    *image_slot=original_image;
    check(initialized && native_calls::proxy_address()==proxy && native_calls::target_slot()==&selected_target);
    std::puts("Testing exact native setting decoders...");
    using namespace theater::editor;
    std::array<unsigned char,160> native_dvar{};
    uint32_t peb=uint32_t(__readgsqword(0x60));
    *reinterpret_cast<uint32_t*>(native_dvar.data()+28)=2;
    auto float_getter=reinterpret_cast<float(*)(void*)>(page+0x4000);
    for(float value:{-10.f,-.1f,0.f,.0001f,.01f,.1f,1.f,3.141592f,10.f,18.f,360.f,100000.f}) {
        *reinterpret_cast<float*>(native_dvar.data()+40)=value;
        *reinterpret_cast<uint64_t*>(native_dvar.data()+64)=encode_float(value,peb);
        float decoded=native_calls::invoke(float_getter,static_cast<void*>(native_dvar.data()));
        std::printf("float %.8g -> %.8g\n",value,decoded);
        check(std::bit_cast<uint32_t>(decoded)==std::bit_cast<uint32_t>(value));
    }
    *reinterpret_cast<uint32_t*>(native_dvar.data()+28)=1;
    auto bool_getter=reinterpret_cast<bool(*)(void*)>(page+0x5000);
    for(bool value:{false,true}) {
        std::printf("bool input %d\n",int(value));
        native_dvar[40]=value;
        *reinterpret_cast<uint64_t*>(native_dvar.data()+64)=encode_bool(value,peb);
        bool decoded=native_calls::invoke(bool_getter,static_cast<void*>(native_dvar.data()));
        std::printf("bool output %d\n",int(decoded));check(decoded==value);
    }
    config original;original.menu_background=false;original.move_speed=.0001f;original.fov=.1f;original.timescale=.01f;
    original.values.fog_enabled=original.values.misc_enabled=true;original.values.sync_sun=true;
    original.values.remove_gun=original.values.remove_blood=original.values.remove_hud=true;
    original.values.mist.tint={.2f,1.7f,4.5f};original.values.sunlight.tint={3,2,1};original.values.sunlight.brightness=32000;
    original.values.gun={-10,2,10};light sample_light;sample_light.type=1;sample_light.look_x=125;sample_light.position={2,3,4};original.values.lights={sample_light,sample_light};
    auto loaded=deserialize(serialize(original));check(serialize(original)==serialize(loaded));
    check(!loaded.menu_background);
    auto legacy=serialize(original);auto pref=legacy.find("  \"menuBackground\": false,\n");check(pref!=std::string::npos);
    legacy.erase(pref,std::strlen("  \"menuBackground\": false,\n"));check(deserialize(legacy).menu_background);
    std::puts("CFG round trip and legacy background preference passed");
    auto rejects=[](const std::string& text) {std::printf("Rejecting %.20s\n",text.c_str());try {deserialize(text);return false;}catch(...) {std::puts("rejected");return true;}};
    check(rejects("{}"));check(rejects("{broken"));
    auto bad=serialize(original);auto start=bad.find("\"fov\": 0.1");check(start!=std::string::npos);bad.replace(start,10,"\"fov\": -1");check(rejects(bad));
    for(auto& file:std::filesystem::directory_iterator("assets/fog-presets")) {
        std::printf("Preset %s\n",file.path().filename().string().c_str());
        std::ifstream in(file.path());std::string text{std::istreambuf_iterator<char>(in),{}};
        auto f=deserialize_fog(text);auto copy=deserialize_fog(serialize_fog(f));check(copy.tint.r==f.tint.r && copy.haze_tint.b==f.haze_tint.b);
    }
    auto lights=deserialize_lights(serialize_lights(original.values.lights));check(lights.size()==2 && lights[0].position.z==4 && lights[0].look_x==125);
    std::vector<light> overflow(26);bool rejected=false;try {deserialize_lights(serialize_lights(overflow));}catch(...) {rejected=true;}check(rejected);
    auto exercise=[] {
        void* sentinel=reinterpret_cast<void*>(0x1234);selected_target=sentinel;
        for(int i=0;i<1000;++i) {
            check(native_calls::invoke(target,i,2,3,4)==i+49);
            check(native_calls::invoke(nested,i)==i+49);
            native_calls::invoke(pointer_target,static_cast<void*>(&selected_target),true);
            check(returns.empty() && selected_target==sentinel);
        }
    };
    std::thread a(exercise),b(exercise);exercise();a.join();b.join();
    VirtualFree(page,0,MEM_RELEASE);
    std::puts("Native proxy and editor tests passed: exact game float/bool decoders, protected writes, CFG round-trip and rejection, all supplied fog presets, lighting limits, native return address, nested calls and three concurrent threads.");
}
