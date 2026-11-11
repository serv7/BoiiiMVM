#pragma once
#include <algorithm>
#include <cstdio>
#include <istream>
#include <string>
#include <vector>

// Names and counts for the bot menu's Specialist page. The data file
// (MVM/Theater/outfit-previews/catalog.txt) was read from the game itself:
// outfit N is body style N with helmet style N, which the game lists under one name.
namespace theater::outfits {
struct specialist {
    int id=-1;
    std::string name;
    std::vector<std::string> names; // by outfit index; empty = the game has no name for it
};
struct catalog {
    std::vector<specialist> specialists;
    const specialist* find(int id) const {
        for(const auto& item:specialists)if(item.id==id)return &item;
        return nullptr;
    }
    // Lines: "S|id|name" and "O|id|index|name". Anything else is ignored.
    static catalog parse(std::istream& in) {
        catalog result;std::string line;
        while(std::getline(in,line)) {
            if(!line.empty() && line.back()=='\r')line.pop_back();
            std::vector<std::string> part;size_t from=0;
            for(;;){auto bar=line.find('|',from);part.push_back(line.substr(from,bar-from));if(bar==std::string::npos)break;from=bar+1;}
            auto number=[](const std::string& text,int& out){
                if(text.empty() || text.size()>4)return false;
                int value=0;for(char c:text){if(c<'0' || c>'9')return false;value=value*10+(c-'0');}
                out=value;return true;
            };
            int id=0,index=0;
            if(part.size()==3 && part[0]=="S" && number(part[1],id)) {
                if(!result.find(id))result.specialists.push_back({id,part[2],{}});
            } else if(part.size()==4 && part[0]=="O" && number(part[1],id) && number(part[2],index) && index<256) {
                for(auto& item:result.specialists)if(item.id==id) {
                    if(int(item.names.size())<=index)item.names.resize(size_t(index)+1);
                    item.names[size_t(index)]=part[3];
                }
            }
        }
        return result;
    }
};
// What the menu shows for one entry: the game's name, or a plain numbered fallback.
inline std::string label(const specialist* who,int index,const char* kind) {
    if(who && index>=0 && index<int(who->names.size()) && !who->names[size_t(index)].empty())return who->names[size_t(index)];
    return std::string(kind)+" "+std::to_string(index+1);
}
// How many entries a list has. The game's own count (known once a match has been loaded) wins
// for the separate lists; the catalog fills in before that.
inline int count(const specialist* who,int game_count) {
    if(game_count>0)return std::min(game_count,256);
    return who?int(who->names.size()):0;
}
inline int outfit_count(const specialist* who,int game_bodies,int game_heads) {
    return std::max(count(who,game_bodies),count(who,game_heads));
}
inline std::string image_name(int specialist_id,int index,bool head) {
    char text[32];std::snprintf(text,sizeof(text),"h%d_%02d%s.jpg",specialist_id,index,head?"_head":"");
    return text;
}
// A body and a head from different outfits are shown as the head close-up above the body
// picture cut off at the shoulders. Returns the two rectangles inside a box of the given size.
struct rect {float x0=0,y0=0,x1=0,y1=0;};
struct mixed_layout {rect head,body;float body_v0=0;};
inline mixed_layout mix(float width,float height) {
    constexpr float picture_aspect=399.f/360.f,cut=.29f,head_share=.40f;
    float w=width,head=w*head_share,body=w*picture_aspect*(1-cut);
    float scale=std::min(1.f,height/(head+body));
    w*=scale;head*=scale;body*=scale;
    float left=(width-w)/2,head_left=(width-head)/2;
    return {{head_left,0,head_left+head,head},{left,head,left+w,head+body},cut};
}
}
