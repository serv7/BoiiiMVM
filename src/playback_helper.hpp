#pragma once
#include <cstdint>
#include <string>

namespace theater::playback_helper {
struct snapshot {
    bool hooked=false;
    bool linked=false;
    unsigned loads=0;
    unsigned skipped=0;
    std::string message;
};
using logger=void(*)(const char*);
// Call after native_calls::initialize and before loading the next film.
bool initialize(uintptr_t game_base,logger output=nullptr);
void shutdown();
snapshot current();
}
