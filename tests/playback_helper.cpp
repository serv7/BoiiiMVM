#include "../src/playback_helper_policy.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace theater::playback_helper::detail;
namespace {
struct memory {
    std::map<uintptr_t,std::vector<unsigned char>> blocks;
    bool operator()(uintptr_t p,void* out,size_t size) const {
        auto at=blocks.upper_bound(p);if(at==blocks.begin())return false;--at;
        if(p<at->first || p-at->first>at->second.size() || size>at->second.size()-(p-at->first))return false;
        std::memcpy(out,at->second.data()+(p-at->first),size);return true;
    }
    template<size_t N> void add(uintptr_t p,const std::array<unsigned char,N>& data){blocks[p]={data.begin(),data.end()};}
    void string(uintptr_t p,const std::string& name,bool heap) {
        auto& object=blocks[p];object.assign(32,0);
        uint64_t size=name.size(),capacity=heap?std::max<uint64_t>(16,size):15;
        std::memcpy(object.data()+16,&size,8);std::memcpy(object.data()+24,&capacity,8);
        if(heap) {
            uintptr_t pointer=p+0x1000;std::memcpy(object.data(),&pointer,8);
            blocks[pointer]={name.begin(),name.end()};blocks[pointer].push_back(0);
        } else {assert(name.size()<=15);std::memcpy(object.data(),name.c_str(),name.size()+1);}
    }
};
}
int main() {
    constexpr uintptr_t host=0x10000000,game=0x20000000;
    memory mem;mem.add(host+loader_rva,loader_prefix);mem.add(host+execute_rva,execute_prefix);
    mem.add(host+host_begin_rva,host_begin_prefix);mem.add(host+loader_call_rva,loader_call);
    mem.add(game+native_begin_rva,native_begin_prefix);
    assert(supported(mem,host,game));
    // Where the real read-only captures are available, independently check
    // every profile byte against them as well as the synthetic rejection cases.
    auto capture_matches=[](const char* path,uintptr_t offset,const auto& expected) {
        if(!std::filesystem::exists(path))return;
        std::ifstream file(path,std::ios::binary);file.seekg(offset);
        std::vector<unsigned char> actual(expected.size());
        file.read(reinterpret_cast<char*>(actual.data()),actual.size());
        assert(file.gcount()==static_cast<std::streamsize>(actual.size()));
        assert(std::equal(actual.begin(),actual.end(),expected.begin()));
    };
    capture_matches("build/test5-live/boiii.exe.text.bin",loader_rva-0x1000,loader_prefix);
    capture_matches("build/test5-live/boiii.exe.text.bin",execute_rva-0x1000,execute_prefix);
    capture_matches("build/test5-live/boiii.exe.text.bin",host_begin_rva-0x1000,host_begin_prefix);
    capture_matches("build/test5-live/boiii.exe.text.bin",loader_call_rva-0x1000,loader_call);
    capture_matches("docs/live.text.bin",native_begin_rva-0x1000,native_begin_prefix);
    capture_matches("docs/live.text.bin",error_rva-0x1000,error_prefix);
    capture_matches("docs/live.text.bin",error_fields_rva-0x1000,error_fields);
    for(auto address:{host+loader_rva,host+execute_rva,host+host_begin_rva,host+loader_call_rva,game+native_begin_rva}) {
        mem.blocks[address][0]^=1;assert(!supported(mem,host,game));mem.blocks[address][0]^=1;
    }
    assert(!supported(mem,host+1,game));
    for(bool heap:{false,true}) {
        mem.string(0x1000,"a.csc",heap);assert(foreign_script(mem,0x1000)==script_kind::client);
        mem.string(0x1000,"a.gsc",heap);assert(foreign_script(mem,0x1000)==script_kind::server);
        mem.string(0x1000,"mvm_corpse.csc",heap);assert(foreign_script(mem,0x1000)==script_kind::helper);
    }
    mem.string(0x1000,"custom_scripts/mvm_corpse.csc",true);assert(foreign_script(mem,0x1000)==script_kind::helper);
    mem.string(0x1000,"custom_scripts\\MVM_CORPSE.CSC",true);assert(foreign_script(mem,0x1000)==script_kind::helper);
    mem.string(0x1000,"other_mvm_corpse.csc",true);assert(foreign_script(mem,0x1000)==script_kind::client);
    uint64_t corrupt=512;std::memcpy(mem.blocks[0x1000].data()+16,&corrupt,8);
    assert(foreign_script(mem,0x1000)==script_kind::invalid);
    assert(foreign_script(mem,0x999999)==script_kind::invalid);
    for(auto kind:{script_kind::invalid,script_kind::server,script_kind::client,script_kind::helper,script_kind::other}) {
        assert(forward_script(false,kind));assert(forward_script(true,kind)==(kind==script_kind::helper));
    }
    pass_context pass;unsigned originals=0,loads=0;
    auto original=[&]{++originals;};auto load=[&]{++loads;assert(pass.depth==1);};
    begin(0,true,pass,original,load);begin(1,false,pass,original,load);
    assert(originals==2 && loads==0 && pass.depth==0);
    begin(1,true,pass,original,[&]{load();begin(1,true,pass,original,load);});
    assert(originals==4 && loads==1 && pass.depth==0);
    try {begin(1,true,pass,original,[&]{assert(pass.depth==1);throw std::runtime_error("fixture failure");});assert(false);}
    catch(const std::runtime_error&){}
    assert(pass.depth==0);
    begin(1,true,pass,original,load);assert(loads==2 && pass.depth==0);
    unsigned failures=0;auto before_originals=originals;
    begin(1,true,pass,original,[&]{
        assert(!guarded_load([]{throw std::runtime_error("compiler fixture");},[&](const char* message){++failures;assert(std::string_view(message)=="compiler fixture");}));
        assert(pass.depth==1);
    });
    assert(originals==before_originals+1 && failures==1 && pass.depth==0);
    assert(!guarded_load([]{throw 9;},[&](const char* message){++failures;assert(std::string_view(message)=="unknown C++ exception");}));
    assert(failures==2);
    // VM diagnostics read the supported CLIENT fields only, never dereference
    // untrusted pointers, and cap output even for a missing string terminator.
    script_error error;
    assert(!read_error(mem,game,0,error));
    assert(!read_error(mem,game,1,error));
    auto put=[&](uintptr_t address,const auto& value) {
        auto bytes=reinterpret_cast<const unsigned char*>(&value);
        mem.blocks[address]={bytes,bytes+sizeof(value)};
    };
    uintptr_t message=0x80000000,code=0x90001234;
    put(game+0x51a3710+0x78,message);put(game+0x51c4f30+0x20,code);
    unsigned char terminal=1;put(game+0x51a386b+0x8a40,terminal);
    std::string reason="parameter 0 is not defined\ncustom_scripts/mvm_corpse.csc";
    mem.blocks[message]={reason.begin(),reason.end()};mem.blocks[message].push_back(0);
    assert(read_error(mem,game,1,error));
    assert(error.terminal && error.code==code);
    assert(std::string_view(error.message.data())=="parameter 0 is not defined custom_scripts/mvm_corpse.csc");
    mem.blocks[message]={'p','a','g','e'}; // An inaccessible following page.
    assert(read_error(mem,game,1,error));
    assert(std::string_view(error.message.data())=="page" && error.code==code);
    mem.blocks[message].assign(4096,'x');
    assert(read_error(mem,game,1,error));assert(std::strlen(error.message.data())==1023);
    mem.blocks.erase(message);assert(!read_error(mem,game,1,error));
    assert(error.message[0]==0);
    std::puts("Playback helper policy passed: captured fingerprints, safe VM error diagnostics, foreign strings, film/client routing, reentrancy and loader failure recovery.");
}
