#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
namespace theater::animations {
// Verified against the supported retail runtime; mod-tools BO3 uses another layout.
constexpr size_t record_size=0xf8,name_offset=0,frames_offset=0x20,bones_offset=0x22,fps_offset=0x50;
using record=std::array<unsigned char,record_size>;
struct saved_animation {uintptr_t address=0;record original{},applied{};std::string replacement;};
class swaps {
    std::map<std::string,saved_animation> saved;
    template<class Adapter> bool capture(const std::string& name,Adapter& memory) {
        auto address=memory.find(name);record value;
        if(!address || !memory.read(name,address,value))return false;
        auto it=saved.find(name);
        if(it!=saved.end())return it->second.address==address && value==it->second.applied;
        saved.emplace(name,saved_animation{address,value,value,{}});return true;
    }
public:
    template<class Adapter> bool replace(const std::string& target,const std::string& source,Adapter& memory) {
        if(target.empty() || source.empty() || target==source || !capture(target,memory) || !capture(source,memory))return false;
        auto& destination=saved.at(target);auto value=saved.at(source).original;
        std::memcpy(value.data()+name_offset,destination.original.data()+name_offset,sizeof(uintptr_t));
        if(!memory.write(destination.address,value))return false;
        destination.applied=value;destination.replacement=source;return true;
    }
    template<class Adapter> bool restore(const std::string& name,Adapter& memory) {
        auto it=saved.find(name);if(it==saved.end() || it->second.replacement.empty())return true;
        auto& value=it->second;record current;
        if(memory.find(name)!=value.address || !memory.read(name,value.address,current) || current!=value.applied || !memory.write(value.address,value.original))return false;
        value.applied=value.original;value.replacement.clear();return true;
    }
    template<class Adapter> bool restore_all(Adapter& memory) {
        bool okay=true;for(const auto& item:saved)if(!restore(item.first,memory))okay=false;return okay;
    }
    std::string replacement(const std::string& name) const {auto it=saved.find(name);return it==saved.end()?std::string{}:it->second.replacement;}
    bool has_changes() const {for(const auto& item:saved)if(!item.second.replacement.empty())return true;return false;}
    void clear(){saved.clear();}
};
}
