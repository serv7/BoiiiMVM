#include "native_calls.hpp"
#include <cstring>
#include <algorithm>

namespace native_calls {
namespace {
uintptr_t proxy=0;
DWORD* tls_index=nullptr;
uint32_t tls_offset=0;
constexpr unsigned char prefix[]={0x50,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x41,0x50,0x41,0x51,0x41,0x52,0x41,0x53,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
bool readable(uintptr_t p,size_t size) {
    MEMORY_BASIC_INFORMATION m{};
    return VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) && m.State==MEM_COMMIT &&
        !(m.Protect&(PAGE_NOACCESS|PAGE_GUARD)) && p+size<=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
}
template<class T> T load(uintptr_t p) { T result;std::memcpy(&result,reinterpret_cast<void*>(p),sizeof(result));return result; }
bool matches(uintptr_t p,const char* bytes,size_t size) { return !std::memcmp(reinterpret_cast<void*>(p),bytes,size); }
bool candidate(uintptr_t p,uintptr_t game) {
    if(!readable(p,0xe0) || std::memcmp(reinterpret_cast<void*>(p),prefix,sizeof(prefix)))return false;
    // Exact layout of the BOIII assembler callstack_stub. Check both indirect
    // callback slots and the complete return path rather than an address alone.
    if(!matches(p+0x45,"\xff\x15\x85\x00\x00\x00",6) ||
       !matches(p+0x8d,"\xff\x15\x45\x00\x00\x00",6) ||
       !matches(p+0x99,"\x48\xb8",2) || load<uintptr_t>(p+0x9b)!=game+0x1056 ||
       !matches(p+0xa3,"\x48\x89\x84\x24\x88\x00\x00\x00",8) ||
       !matches(p+0xc3,"\x48\x83\xc4\x08\xff\x64\x24\xf8",8))return false;
    auto getter=load<uintptr_t>(p+0xd0),store=load<uintptr_t>(p+0xd8);
    // BOIII changes PEB.ImageBaseAddress to the mapped game. Consequently
    // GetModuleHandle(nullptr) names BlackOps3, not the client that owns these
    // callbacks. Resolve their module by address through the loader's list.
    HMODULE owner=nullptr,store_owner=nullptr;
    constexpr DWORD flags=GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if(!GetModuleHandleExW(flags,reinterpret_cast<LPCWSTR>(getter),&owner) ||
       !GetModuleHandleExW(flags,reinterpret_cast<LPCWSTR>(store),&store_owner) || owner!=store_owner)return false;
    auto client=reinterpret_cast<uintptr_t>(owner);
    if(client==game || !readable(client,sizeof(IMAGE_DOS_HEADER)))return false;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(client);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096)return false;
    auto header=client+dos->e_lfanew;
    if(!readable(header,sizeof(IMAGE_NT_HEADERS64)))return false;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(header);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;
    auto client_size=nt->OptionalHeader.SizeOfImage;
    if(getter<client || getter+27>client+client_size || store<client || store>=client+client_size || !readable(getter,27))return false;
    if(!matches(getter,"\x8b\x05",2) ||
       !matches(getter+6,"\x65\x48\x8b\x0c\x25\x58\x00\x00\x00\x48\x8b\x04\xc1",13))return false;
    uint32_t offset;
    if(matches(getter+19,"\x48\x8b\x40",3) && load<unsigned char>(getter+23)==0xc3)
        offset=load<unsigned char>(getter+22);
    else if(matches(getter+19,"\x48\x8b\x80",3) && load<unsigned char>(getter+26)==0xc3)
        offset=load<uint32_t>(getter+22);
    else return false;
    if(load<unsigned char>(getter+21)==0x40 && offset>0x7f)return false;
    auto index=getter+6+load<int32_t>(getter+2);
    if(!readable(index,4) || load<DWORD>(index)>=64 || offset>0x10000 || offset%8)return false;
    proxy=p;tls_index=reinterpret_cast<DWORD*>(index);tls_offset=offset;return true;
}
}
bool initialize(uintptr_t game) {
    proxy=0;tls_index=nullptr;
    if(!readable(game+0x1054,16) || !matches(game+0x1054,"\xff\xcc\x90\xe9",4))return false;
    uintptr_t address=0;
    MEMORY_BASIC_INFORMATION m{};
    while(VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m))) {
        auto start=reinterpret_cast<uintptr_t>(m.BaseAddress);
        auto next=start+m.RegionSize;
        if(next<=address)break;
        address=next;
        if(m.State!=MEM_COMMIT || m.Type!=MEM_PRIVATE || !(m.Protect&0xf0) || (m.Protect&PAGE_GUARD) || m.RegionSize>8*1024*1024)continue;
        auto data=reinterpret_cast<const unsigned char*>(start);
        for(size_t i=0;i+0xe0<=m.RegionSize;++i) {
            if(data[i]==prefix[0] && data[i+1]==prefix[1] && candidate(start+i,game))return true;
        }
    }
    return false;
}
uintptr_t proxy_address() {return proxy;}
void** target_slot() {
    auto blocks=reinterpret_cast<uintptr_t*>(__readgsqword(0x58));
    return reinterpret_cast<void**>(blocks[*tls_index]+tls_offset);
}
}
