#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace theater::memory {
// VirtualQuery walks the whole region to report its size, and BO3's image/data
// regions are huge. Calling it on every window message and several times per
// frame saturated the input thread and cost frame time. Checks now reuse a
// region confirmed readable on the same thread within the last quarter second.
struct region {uintptr_t begin=0,end=0;DWORD protect=0;};
inline bool committed(uintptr_t p,region& out) {
    struct entry {region value;ULONGLONG expires=0;};
    thread_local std::array<entry,8> recent{};
    thread_local unsigned next=0;
    if(!p)return false;
    const ULONGLONG now=GetTickCount64();
    for(const auto& item:recent)
        if(now<item.expires && p>=item.value.begin && p<item.value.end){out=item.value;return true;}
    MEMORY_BASIC_INFORMATION m{};
    if(!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) || m.State!=MEM_COMMIT ||
        (m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
    out={reinterpret_cast<uintptr_t>(m.BaseAddress),reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize,m.Protect};
    recent[next++%recent.size()]={out,now+250};
    return true;
}
inline bool readable(uintptr_t p,size_t n=1) {
    region value;
    return p+n>=p && committed(p,value) && p+n<=value.end;
}
inline bool writable(uintptr_t p,size_t n) {
    region value;
    return p+n>=p && committed(p,value) && p+n<=value.end &&
        (value.protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
}
