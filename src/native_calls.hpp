#pragma once
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <utility>
#include <type_traits>

// Reuse BOIII's callstack proxy, including its native return stub and TLS stack.
// Direct calls from a DLL can leave protected theater functions spinning forever.
namespace native_calls {
bool initialize(uintptr_t game_base);
uintptr_t proxy_address();
void** target_slot();

struct target_scope {
    void** slot;
    void* previous;
    target_scope(void** destination,void* target):slot(destination),previous(*destination) { *slot=target; }
    ~target_scope() { *slot=previous; }
};
template<class F,class... Args> decltype(auto) invoke(F target,Args&&... args) {
    static_assert((!std::is_floating_point_v<std::decay_t<Args>> && ...),"BOIII call proxy does not preserve floating-point arguments");
    target_scope scope(target_slot(),reinterpret_cast<void*>(target));
    // Only integer/pointer arguments are supported: the client's proxy does not
    // preserve volatile XMM argument registers around its TLS bookkeeping calls.
    return reinterpret_cast<F>(proxy_address())(std::forward<Args>(args)...);
}
}
