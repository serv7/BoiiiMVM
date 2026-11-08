#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <span>
#include <string_view>
#include <utility>

namespace theater::playback_helper::detail {
// Supported BOIII executable, captured and checked against script.cpp. These
// contain RIP-relative operands, so the fingerprints survive ASLR unchanged.
inline constexpr uintptr_t loader_rva=0x12db8b, execute_rva=0x12b14b;
inline constexpr uintptr_t host_begin_rva=0x131192, loader_call_rva=0x13124c;
inline constexpr uintptr_t native_begin_rva=0x12c7df0;
// Scr_ErrorInternal is the common error dispatcher, after error_message has
// been set. Its complete body was checked: it can longjmp, but has no protected
// caller check. Observe it without the native-call proxy or live RAII scopes.
inline constexpr uintptr_t error_rva=0x12ea4c0, error_fields_rva=0x12ea538;
inline constexpr std::array<unsigned char,48> error_prefix={
    0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x30,0x4c,0x63,0xd1,0x48,0x8d,0x35,0x22,0x5b,0xd1,0xfe,0x48,0x8d,0x2d,0x4b,0x28,0xed,0x03,0x49,0x8b,0xfa,0x49,0x8b,0xda,0x48,0x6b,0xff,0x78,0x0f};
inline constexpr std::array<unsigned char,27> error_fields={
    0x4c,0x8b,0x8c,0x37,0x10,0x37,0x1a,0x05,0x48,0xc1,0xe3,0x05,0x48,0x8d,0x05,0x8d,0xa7,0xcc,0x01,0x48,0x8b,0x94,0x33,0x30,0x4f,0x1c,0x05};
inline constexpr std::array<unsigned char,48> loader_prefix={
    0x56,0x48,0x83,0xec,0x70,0x48,0x8b,0x05,0xe9,0xa1,0x42,0x00,0x48,0x31,0xe0,0x48,0x89,0x44,0x24,0x68,0x48,0x8d,0x15,0x6c,0x50,0x34,0x00,0x48,0x8d,0x74,0x24,0x48,0x48,0x89,0xf1,0xe8,0x2d,0xa8,0xed,0xff,0x48,0x89,0xf1,0x31,0xd2,0xe8,0xad,0xe4};
inline constexpr std::array<unsigned char,48> execute_prefix={
    0x55,0x41,0x57,0x41,0x56,0x56,0x57,0x53,0x48,0x83,0xec,0x78,0x48,0x8d,0x6c,0x24,0x70,0x48,0xc7,0x45,0x00,0xfe,0xff,0xff,0xff,0x48,0x89,0xce,0x48,0x8d,0x05,0x24,0xd3,0x35,0x00,0x48,0x89,0x45,0xd0,0x41,0xbe,0x04,0x00,0x00,0x00,0x4c,0x89,0x75};
inline constexpr std::array<unsigned char,48> host_begin_prefix={
    0x55,0x56,0x57,0x53,0x48,0x83,0xec,0x28,0x48,0x8d,0x6c,0x24,0x20,0x48,0xc7,0x45,0x00,0xfe,0xff,0xff,0xff,0x89,0xd6,0x89,0xcf,0x48,0x8d,0x0d,0x46,0x2e,0x41,0x00,0xe8,0xc1,0xe4,0xed,0xff,0xe8,0x96,0xf7,0xff,0xff,0xe8,0xa1,0x47,0x03,0x00,0xbb};
inline constexpr std::array<unsigned char,5> loader_call={0xe8,0x3a,0xc9,0xff,0xff};
inline constexpr std::array<unsigned char,48> native_begin_prefix={
    0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0x63,0xe9,0x48,0x63,0xfa,0x8b,0xcd,0xe8,0x6b,0x47,0x00,0x00,0x48,0x8d,0x05,0x34,0xa0,0xa8,0x03,0x45,0x33,0xff,0xc7};

template<class Reader,size_t N> bool match(Reader&& read,uintptr_t address,const std::array<unsigned char,N>& bytes) {
    std::array<unsigned char,N> actual{};
    return read(address,actual.data(),actual.size()) && actual==bytes;
}
template<class Reader> bool supported(Reader&& read,uintptr_t host,uintptr_t game) {
    return match(read,host+loader_rva,loader_prefix) && match(read,host+execute_rva,execute_prefix) &&
        match(read,host+host_begin_rva,host_begin_prefix) && match(read,host+loader_call_rva,loader_call) &&
        match(read,game+native_begin_rva,native_begin_prefix);
}

struct script_error {
    std::array<char,1024> message{};
    uintptr_t code=0;
    bool terminal=false;
};
template<class Reader> bool read_error(Reader&& read,uintptr_t game,int instance,script_error& out) {
    out={};
    if(instance!=1)return false;
    uintptr_t text=0;
    if(!read(game+0x51a3710+0x78,&text,sizeof(text)) || !text)return false;
    // Bounded reads also work when a short string ends at an inaccessible page.
    for(size_t i=0;i+1<out.message.size();++i) {
        char value=0;
        if(!read(text+i,&value,1))break;
        if(!value)break;
        out.message[i]=(value=='\r' || value=='\n')?' ':value;
    }
    read(game+0x51c4f30+0x20,&out.code,sizeof(out.code));
    unsigned char terminal=0;
    read(game+0x51a386b+0x8a40,&terminal,1);
    out.terminal=terminal!=0;
    return out.message[0]!=0;
}

enum class script_kind {invalid,server,client,helper,other};
inline char lower(char value){return value>='A' && value<='Z'?char(value-'A'+'a'):value;}
inline bool ends_with(std::string_view name,std::string_view suffix) {
    if(name.size()<suffix.size())return false;
    name.remove_prefix(name.size()-suffix.size());
    for(size_t i=0;i<name.size();++i)if(lower(name[i])!=suffix[i])return false;
    return true;
}
inline script_kind classify(std::string_view name) {
    if(ends_with(name,".gsc"))return script_kind::server;
    if(!ends_with(name,".csc"))return script_kind::other;
    auto last=name.find_last_of("/\\");auto leaf=last==std::string_view::npos?name:name.substr(last+1);
    return leaf.size()==14 && ends_with(leaf,"mvm_corpse.csc")?script_kind::helper:script_kind::client;
}
// Read the verified release-MSVC x64 basic_string layout. Never construct,
// mutate, free or pass an owning STL object across module/CRT boundaries.
template<class Reader> script_kind foreign_script(Reader&& read,uintptr_t object) {
    std::array<unsigned char,32> value{};
    if(!object || !read(object,value.data(),value.size()))return script_kind::invalid;
    uint64_t size=0,capacity=0;std::memcpy(&size,value.data()+16,8);std::memcpy(&capacity,value.data()+24,8);
    if(!size || size>511 || size>capacity || capacity>0x100000)return script_kind::invalid;
    std::array<char,512> name{};
    if(capacity<16) {
        if(size>15)return script_kind::invalid;
        std::memcpy(name.data(),value.data(),size+1);
    } else {
        uintptr_t pointer=0;std::memcpy(&pointer,value.data(),8);
        if(!pointer || !read(pointer,name.data(),size+1))return script_kind::invalid;
    }
    if(name[size])return script_kind::invalid;
    for(size_t i=0;i<size;++i)if(name[i]<32 || static_cast<unsigned char>(name[i])>126)return script_kind::invalid;
    return classify(std::string_view(name.data(),size_t(size)));
}
inline bool forward_script(bool client_pass,script_kind kind) {
    // Only this playback helper is executed by our extra pass. Other mods and
    // all GSCs keep their existing BOIII load path.
    return !client_pass || kind==script_kind::helper;
}
struct pass_context {unsigned depth=0;};
struct pass_scope {
    pass_context& state;
    explicit pass_scope(pass_context& value):state(value){++state.depth;}
    ~pass_scope(){--state.depth;}
};
template<class Original,class Loader> void begin(int instance,bool playback,pass_context& context,Original&& original,Loader&& loader) {
    std::forward<Original>(original)();
    if(instance!=1 || !playback || context.depth)return;
    pass_scope scope(context);
    std::forward<Loader>(loader)();
}
template<class Loader,class Failure> bool guarded_load(Loader&& loader,Failure&& failure) {
    try {std::forward<Loader>(loader)();return true;}
    catch(const std::exception& error){std::forward<Failure>(failure)(error.what());}
    catch(...){std::forward<Failure>(failure)("unknown C++ exception");}
    return false;
}
}
