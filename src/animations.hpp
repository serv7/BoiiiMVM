#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
namespace theater::animations {
struct entry {std::string name,replacement;};
struct active_clip {std::string name;int entity=0;float time=0,weight=0;};
struct snapshot {bool ready=false;std::vector<entry> entries;std::string message="Load a film to scan BO3 death animations.";
    std::shared_ptr<const std::vector<entry>> catalog;std::vector<active_clip> active;bool loading=false;
    unsigned active_scans=0,address_scans=0;int active_tick=-1;double active_scan_ms=0;};
enum class operation {refresh,replace,restore,restore_all,inspect_active};
struct request {operation type;std::string target,source;int tick=-1;};
bool initialize(uintptr_t base);
void begin();
void update(bool paused);
void end();
void shutdown();
snapshot current();
void enqueue(request value);
void draw_menu(float scale,bool paused);
void draw_weapon_controls(float scale);
}
