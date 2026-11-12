#include "outfit_menu.hpp"
#include "outfit_catalog.hpp"
#include "memory_regions.hpp"
#include <Windows.h>
#include <gdiplus.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#pragma comment(lib,"gdiplus.lib")

// The bot menu is drawn by boiii.exe with its own copy of Dear ImGui (1.93 WIP). The page drawn
// here therefore calls that copy, not the plugin's: every address below is checked against the
// bytes of the one supported client build before anything is changed.
namespace theater::outfit_menu {
namespace {
namespace sites {
#include "outfit_menu_sites.inc"
}
using Microsoft::WRL::ComPtr;
struct vec2 {float x=0,y=0;};
// std::string as the client lays it out. Only short values go through it (15 characters at
// most), so the text lives inside the object and no heap memory changes hands.
struct host_string {
    char text[16]{};size_t size=0,capacity=15;
    explicit host_string(const std::string& value){size=std::min<size_t>(value.size(),15);std::memcpy(text,value.data(),size);}
};
// One specialist in the menu's own list (from boiii/scriptdata/botmod_characters.txt).
struct host_entry {
    int id;int padding;
    union {char text[16];const char* pointer;} name;
    size_t name_size,name_capacity;
    int bodies,heads;
};
static_assert(sizeof(host_entry)==0x30);
struct host_vector {const host_entry* begin;const host_entry* end;const host_entry* capacity;};
struct texture_ref {void* data=nullptr;uint64_t id=0;}; // ImTextureRef
constexpr uintptr_t current_window_offset=0x14d0,cursor_offset=0x118,draw_list_offset=0x2c8;
constexpr int borders=1,no_scrollbar=8,no_scroll_with_mouse=16;
const char* const folder="MVM/Theater/outfit-previews";
const char* const settings_file="MVM/outfit_menu.cfg";

uintptr_t host=0,context_pointer=0;
void(*log)(const char*)=nullptr;
void* stub=nullptr;
bool patched=false,separate=false,announced=false,editing_head=false;
unsigned char original[12]{};
outfits::catalog names;

template<class T> T call(const sites::site& where){return reinterpret_cast<T>(host+where.rva);}
int get_int(const char* name){return call<int(*)(const char*)>(sites::get_int)(name);}
void set_value(const char* name,int value){host_string text(std::to_string(value));call<void(*)(const char*,host_string*)>(sites::set_value)(name,&text);}
void run_command(const char* command){host_string text(command);call<void(*)(const host_string*)>(sites::run_command)(&text);}
void header(const char* label){call<void(*)(const char*)>(sites::header)(label);}
bool begin_child(const char* id,vec2 size,int child_flags,int window_flags){return call<bool(*)(const char*,const vec2*,int,int)>(sites::begin_child)(id,&size,child_flags,window_flags);}
void end_child(){call<void(*)()>(sites::end_child)();}
bool selectable(const std::string& label,bool selected,vec2 size={}){return call<bool(*)(const char*,bool,int,const vec2*)>(sites::selectable)(label.c_str(),selected,0,&size);}
void same_line(){call<void(*)(float,float)>(sites::same_line)(0,-1);}
void begin_group(){call<void(*)()>(sites::begin_group)();}
void end_group(){call<void(*)()>(sites::end_group)();}
void spacing(){call<void(*)()>(sites::spacing)();}
void text(const std::string& value){call<void(*)(const char*,...)>(sites::text)("%s",value.c_str());}
void text_wrapped(const char* value){call<void(*)(const char*,...)>(sites::text_wrapped)("%s",value);}
void text_disabled(const char* value){call<void(*)(const char*,...)>(sites::text_disabled)("%s",value);}
bool button(const char* label,vec2 size){return call<bool(*)(const char*,const vec2*)>(sites::button)(label,&size);}
bool checkbox(const char* label,bool* value){return call<bool(*)(const char*,bool*)>(sites::checkbox)(label,value);}
bool hovered(){return call<bool(*)(int)>(sites::item_hovered)(0);}
void tooltip(const char* value){call<void(*)(const char*,...)>(sites::set_tooltip)("%s",value);}
vec2 available(){vec2 value;call<vec2*(*)(vec2*)>(sites::content_avail)(&value);return value;}
float line_height(){return call<float(*)()>(sites::line_height)();}
uintptr_t current_window() {
    auto context=*reinterpret_cast<uintptr_t*>(context_pointer);
    return context?*reinterpret_cast<uintptr_t*>(context+current_window_offset):0;
}

// ---- pictures -------------------------------------------------------------------------------
std::mutex device_guard;
ComPtr<ID3D11Device> device;
struct picture {ComPtr<ID3D11ShaderResourceView> view;uint64_t used=0;bool failed=false;};
std::map<std::string,picture> pictures;
uint64_t frame=0;
ULONG_PTR gdi_token=0;

ComPtr<ID3D11ShaderResourceView> load(const std::string& name) {
    ComPtr<ID3D11Device> target;
    {std::lock_guard lock(device_guard);target=device;}
    if(!target)return nullptr;
    if(!gdi_token){Gdiplus::GdiplusStartupInput input;if(Gdiplus::GdiplusStartup(&gdi_token,&input,nullptr)!=Gdiplus::Ok){gdi_token=0;return nullptr;}}
    const auto path=std::filesystem::path(folder)/name;
    std::error_code error;
    if(!std::filesystem::is_regular_file(path,error))return nullptr;
    Gdiplus::Bitmap image(path.c_str());
    const UINT width=image.GetWidth(),height=image.GetHeight();
    if(image.GetLastStatus()!=Gdiplus::Ok || !width || !height || width>1024 || height>1024)return nullptr;
    Gdiplus::BitmapData data{};Gdiplus::Rect all(0,0,int(width),int(height));
    if(image.LockBits(&all,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data)!=Gdiplus::Ok)return nullptr;
    std::vector<unsigned char> pixels(size_t(width)*height*4);
    for(UINT y=0;y<height;++y)
        std::memcpy(pixels.data()+size_t(y)*width*4,static_cast<unsigned char*>(data.Scan0)+ptrdiff_t(y)*data.Stride,size_t(width)*4);
    image.UnlockBits(&data);
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=pixels.data();initial.SysMemPitch=width*4;
    ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;
    if(FAILED(target->CreateTexture2D(&desc,&initial,&texture)) || FAILED(target->CreateShaderResourceView(texture.Get(),nullptr,&view)))return nullptr;
    return view;
}
// Pictures are decoded the first time they are shown and kept, so moving over the list again is
// instant. The menu's draw commands hold raw pointers until the frame is rendered, so only
// pictures not used for a few frames are ever dropped.
ID3D11ShaderResourceView* picture_for(const std::string& name) {
    if(pictures.size()>160) {
        for(auto item=pictures.begin();item!=pictures.end() && pictures.size()>120;)
            item=item->second.used+3<frame?pictures.erase(item):std::next(item);
    }
    auto& item=pictures[name];
    item.used=frame;
    if(!item.view && !item.failed){item.view=load(name);item.failed=!item.view;}
    return item.view.Get();
}
void image(uintptr_t window,ID3D11ShaderResourceView* view,outfits::rect box,float v0=0) {
    if(!view)return;
    const vec2 origin=*reinterpret_cast<vec2*>(window+cursor_offset);
    auto list=*reinterpret_cast<void**>(window+draw_list_offset);
    if(!list)return;
    texture_ref texture{nullptr,reinterpret_cast<uint64_t>(view)};
    vec2 a{origin.x+box.x0,origin.y+box.y0},b{origin.x+box.x1,origin.y+box.y1},uv0{0,v0},uv1{1,1};
    call<void(*)(void*,texture_ref*,const vec2*,const vec2*,const vec2*,const vec2*,unsigned)>(sites::add_image)(list,&texture,&a,&b,&uv0,&uv1,0xffffffffu);
}

// ---- the page -------------------------------------------------------------------------------
std::string host_name(const host_entry& entry) {
    const char* text=entry.name_capacity>=16?entry.name.pointer:entry.name.text;
    return text && entry.name_size<64?std::string(text,entry.name_size):std::string();
}
void save_settings(){std::ofstream out(settings_file,std::ios::trunc);out<<"separate="<<(separate?1:0)<<"\n";}
void load_settings(){std::ifstream in(settings_file);std::string line;while(std::getline(in,line))if(line.rfind("separate=",0)==0)separate=line.size()>9 && line[9]=='1';}

// A list of names; returns the entry under the mouse, -1 when none.
int name_list(const char* id,vec2 size,const outfits::specialist* who,const char* kind,int total,int chosen,int& picked) {
    int under=-1;
    if(begin_child(id,size,borders,0)) {
        for(int i=0;i<total;++i) {
            if(selectable(outfits::label(who,i,kind)+"##"+id+std::to_string(i),i==chosen))picked=i;
            if(hovered())under=i;
        }
    }
    end_child();
    return under;
}
void draw_outfits(const host_entry& entry,float width,float height,float line) {
    const auto* who=names.find(entry.id);
    const int body=get_int("botmod_body"),head=get_int("botmod_head");
    const float gap=8,list_width=std::max(140.f,line*5.9f);
    int show_body=body,show_head=head,total=0;
    if(!separate) {
        // One entry is the matching body and head. A mixed pair set earlier in separate mode
        // is left alone until an outfit is picked; it just has no highlighted entry.
        total=outfits::outfit_count(who,entry.bodies,entry.heads);
        int picked=-1;
        const int under=name_list("outfits",{list_width,height},who,"Outfit",total,body==head?body:-1,picked);
        if(under>=0)show_body=show_head=under;
        if(picked>=0){set_value("botmod_body",picked);set_value("botmod_head",picked);}
    } else {
        // One tall list at a time: "Body" and "Head" above it choose which part it changes.
        const int bodies=outfits::count(who,entry.bodies),heads=outfits::count(who,entry.heads);
        total=bodies+heads;
        const float half=(list_width-gap)/2;
        begin_group();
        if(selectable("Body##part",!editing_head,{half,0}))editing_head=false;
        same_line();
        if(selectable("Head##part",editing_head,{half,0}))editing_head=true;
        int picked=-1;
        const vec2 size{list_width,std::max(line*4,height-line-4)};
        if(!editing_head) {
            const int under=name_list("bodies",size,who,"Body",bodies,body,picked);
            if(under>=0)show_body=under;
            if(picked>=0)set_value("botmod_body",picked);
        } else {
            const int under=name_list("heads",size,who,"Head",heads,head,picked);
            if(under>=0)show_head=under;
            if(picked>=0)set_value("botmod_head",picked);
        }
        end_group();
    }
    same_line();
    begin_group();
    float picture_width=std::max(110.f,width-list_width-2*gap);
    float picture_height=picture_width*399.f/360.f;
    const float room=height-(show_body==show_head?1:2)*line;
    if(picture_height>room && room>60){picture_height=room;picture_width=picture_height*360.f/399.f;}
    if(begin_child("preview",{picture_width,picture_height},0,no_scrollbar|no_scroll_with_mouse)) {
        if(const auto window=current_window()) {
            bool drawn=false;
            if(show_body==show_head) {
                auto view=picture_for(outfits::image_name(entry.id,show_body,false));
                image(window,view,{0,0,picture_width,picture_height});drawn=view!=nullptr;
            } else {
                const auto parts=outfits::mix(picture_width,picture_height);
                auto top=picture_for(outfits::image_name(entry.id,show_head,true));
                auto bottom=picture_for(outfits::image_name(entry.id,show_body,false));
                image(window,top,parts.head);image(window,bottom,parts.body,parts.body_v0);
                drawn=top || bottom;
            }
            if(!drawn)text_disabled(total?"No picture for this one":"Load into a match to list the skins");
        }
    }
    end_child();
    if(show_body==show_head)text(outfits::label(who,show_body,"Outfit"));
    else {text("Body: "+outfits::label(who,show_body,"Body"));text("Head: "+outfits::label(who,show_head,"Head"));}
    end_group();
}
void draw(const host_vector* list) {
    ++frame;
    const int chosen=get_int("botmod_specialist");
    header("Specialist");
    const float line=line_height();
    const vec2 room=available();
    const float names_width=std::max(100.f,line*4.4f),height=std::max(line*11,room.y-line*4.8f);
    const host_entry* selected=nullptr;
    const bool usable=list && list->begin && list->end>=list->begin && list->end-list->begin<=64;
    if(usable)for(auto entry=list->begin;entry!=list->end;++entry)if(entry->id==chosen)selected=entry;
    if(begin_child("specialists",{names_width,height},borders,0)) {
        if(selectable("Random",chosen<0))set_value("botmod_specialist",-1);
        if(usable)for(auto entry=list->begin;entry!=list->end;++entry) {
            const auto* known=names.find(entry->id);
            if(selectable((known && !known->name.empty()?known->name:host_name(*entry))+"##"+std::to_string(entry->id),entry->id==chosen)) {
                set_value("botmod_specialist",entry->id);set_value("botmod_body",0);set_value("botmod_head",0);
            }
        }
    }
    end_child();
    same_line();
    begin_group();
    if(selected)draw_outfits(*selected,room.x-names_width-8,height,line);
    else text_disabled("Random: bots get a random specialist and skins.");
    end_group();
    spacing();
    if(selected) {
        bool value=separate;
        if(checkbox("Separate Head & Body Selection",&value)){separate=value;save_settings();}
        if(hovered())tooltip("Off: one list of complete outfits. On: pick the body and the head from different outfits.");
    }
    vec2 wide=available();wide.y=0;
    if(button("Apply to me",wide))run_command("botmod_applyme");
    if(hovered())tooltip("You wear this specialist and skin, including after respawning");
    text_wrapped("Applies to bots spawned from now on (or use \"Give existing bots the current loadout\").");
    spacing();
    if(button("Unfreeze bots",wide))run_command("botmod_unfreeze");
    if(hovered())tooltip("Lets every living bot move and fight. Killed bots respawn on their spot, frozen again.");
    if(!announced){announced=true;if(log)log("Bot menu: Specialist page drawn with outfit names and pictures");}
}
// Reached from the stub in place of the client's own page code; nothing may escape into it.
void entry(const host_vector* list) {
    try {draw(list);} catch(...) {}
}
bool matches(const sites::site& where) {
    return memory::readable(host+where.rva,where.length) && !std::memcmp(reinterpret_cast<void*>(host+where.rva),where.bytes,where.length);
}
bool write_code(void* target,const void* bytes,size_t size) {
    DWORD old=0;
    if(!VirtualProtect(target,size,PAGE_EXECUTE_READWRITE,&old))return false;
    std::memcpy(target,bytes,size);VirtualProtect(target,size,old,&old);
    FlushInstructionCache(GetCurrentProcess(),target,size);return true;
}
}

void set_device(ID3D11Device* value) {
    std::lock_guard lock(device_guard);
    if(device.Get()!=value){device=value;}
}

bool initialize(uintptr_t module,void(*logger)(const char*)) {
    log=logger;host=module;
    if(!host || patched)return false;
    for(const auto* where:{&sites::patch_site,&sites::resume,&sites::get_int,&sites::set_value,&sites::run_command,&sites::header,
        &sites::toggle,&sites::begin_child,&sites::end_child,&sites::selectable,&sites::same_line,&sites::begin_group,&sites::end_group,
        &sites::spacing,&sites::text,&sites::text_wrapped,&sites::text_disabled,&sites::button,&sites::item_hovered,&sites::set_tooltip,
        &sites::content_avail,&sites::line_height,&sites::add_image,&sites::checkbox})
        if(!matches(*where)){if(log)log("Bot menu outfit page unavailable: client build not recognised");return false;}
    // GetContentRegionAvail starts "mov rax,rcx; mov r8,[rip+x]": x locates the client's ImGui context pointer.
    int displacement=0;std::memcpy(&displacement,reinterpret_cast<void*>(host+sites::content_avail.rva+6),4);
    context_pointer=host+sites::content_avail.rva+10+displacement;
    if(!memory::readable(context_pointer,8)){if(log)log("Bot menu outfit page unavailable: context not found");return false;}
    {std::ifstream in(std::filesystem::path(folder)/"catalog.txt");names=outfits::catalog::parse(in);}
    load_settings();
    // The page's code runs from patch_site to resume inside the menu function, with the menu's
    // copy of its specialist list at [rbp+0xd0]. The stub hands that list to entry() and
    // continues at resume, where the client frees the copy and ends the tab.
    unsigned char code[]={0x48,0x8d,0x8d,0xd0,0,0,0, 0x48,0xb8,0,0,0,0,0,0,0,0, 0xff,0xd0, 0x48,0xb8,0,0,0,0,0,0,0,0, 0xff,0xe0};
    const auto target=reinterpret_cast<uintptr_t>(&entry),back=host+sites::resume.rva;
    std::memcpy(code+9,&target,8);std::memcpy(code+21,&back,8);
    stub=VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!stub)return false;
    std::memcpy(stub,code,sizeof(code));FlushInstructionCache(GetCurrentProcess(),stub,sizeof(code));
    unsigned char jump[12]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
    const auto address=reinterpret_cast<uintptr_t>(stub);std::memcpy(jump+2,&address,8);
    auto site=reinterpret_cast<void*>(host+sites::patch_site.rva);
    std::memcpy(original,site,sizeof(original));
    if(!write_code(site,jump,sizeof(jump))){if(log)log("Bot menu outfit page unavailable: could not patch");return false;}
    patched=true;
    if(log) {
        size_t outfits_known=0;for(const auto& who:names.specialists)outfits_known+=who.names.size();
        const std::string message="Bot menu outfit page ready: "+std::to_string(names.specialists.size())+" specialists, "+std::to_string(outfits_known)+" outfits in the catalog";
        log(message.c_str());
    }
    return true;
}

void shutdown() {
    if(patched){write_code(reinterpret_cast<void*>(host+sites::patch_site.rva),original,sizeof(original));patched=false;}
    pictures.clear();
    {std::lock_guard lock(device_guard);device.Reset();}
    if(gdi_token){Gdiplus::GdiplusShutdown(gdi_token);gdi_token=0;}
}
}
