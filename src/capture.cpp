#include "capture.hpp"
#include "editor.hpp"
#include "backend.hpp"
#include "vendor/json.hpp"
#include <Windows.h>
#include <dxgi.h>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <atomic>

namespace theater::capture {
namespace {
std::mutex mutex;
std::condition_variable room,ready;
settings options;
status current;
std::deque<std::vector<unsigned char>> queue;
std::thread worker;
bool close_worker=false;
bool toggle_pending=false;
std::atomic<bool> post_effect_source=false;
std::atomic<bool> post_effect_seen=false;
ID3D11Texture2D* staging=nullptr;
UINT width=0,height=0;
DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
UINT expected_width=0,expected_height=0;
DXGI_FORMAT expected_format=DXGI_FORMAT_UNKNOWN;
std::wstring ffmpeg_path(){
    wchar_t result[32768]{};
    auto length=GetFullPathNameW(L"MVM\\Theater\\ffmpeg.exe",DWORD(std::size(result)),result,nullptr);
    if(length && length<std::size(result)){
        auto attributes=GetFileAttributesW(result);
        if(attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY))return result;
    }
    length=SearchPathW(nullptr,L"ffmpeg.exe",nullptr,DWORD(std::size(result)),result,nullptr);
    if(length && length<std::size(result))return result;
    return {};
}
std::wstring quote(const std::wstring& value){return L"\""+value+L"\"";}
void discard_staging(){if(staging){staging->Release();staging=nullptr;}width=height=0;format=DXGI_FORMAT_UNKNOWN;}
std::wstring new_filename(int codec){
    std::filesystem::create_directories("MVM/Theater/recordings");
    SYSTEMTIME now;GetLocalTime(&now);wchar_t name[160];
    swprintf_s(name,L"MVM\\Theater\\recordings\\bo3-%04d%02d%02d-%02d%02d%02d-%03d.%s",
        now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds,codec==1?L"mkv":L"mov");
    return name;
}
bool write_all(HANDLE pipe,const unsigned char* data,size_t bytes){
    while(bytes){DWORD sent=0;DWORD chunk=DWORD(std::min<size_t>(bytes,1<<20));
        if(!WriteFile(pipe,data,chunk,&sent,nullptr) || !sent)return false;data+=sent;bytes-=sent;}
    return true;
}
void encode(std::wstring executable,std::wstring file,UINT w,UINT h,DXGI_FORMAT pixel,settings cfg){
    SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};HANDLE read=nullptr,write=nullptr;
    if(!CreatePipe(&read,&write,&security,1<<20)){std::lock_guard lock(mutex);current.message="Could not create encoder pipe";current.finalizing=false;return;}
    SetHandleInformation(write,HANDLE_FLAG_INHERIT,0);
    std::wstring command=quote(executable)+L" -hide_banner -loglevel error -y -f rawvideo -pixel_format "+
        (pixel==DXGI_FORMAT_R8G8B8A8_UNORM?L"rgba":L"bgra")+L" -video_size "+std::to_wstring(w)+L"x"+std::to_wstring(h)+
        L" -framerate "+std::to_wstring(cfg.fps)+L" -i pipe:0 -an";
    command+=cfg.codec==0?L" -c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le":
        cfg.codec==2?L" -c:v prores_ks -profile:v 4 -pix_fmt yuv444p10le":
        L" -c:v ffv1 -level 3 -pix_fmt yuv444p10le";
    command+=L" "+quote(file);
    auto log_path=std::filesystem::path(file).parent_path()/L"ffmpeg-last.log";
    HANDLE log=CreateFileW(log_path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    STARTUPINFOW start{};start.cb=sizeof(start);start.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    start.wShowWindow=SW_HIDE;start.hStdInput=read;start.hStdOutput=log==INVALID_HANDLE_VALUE?GetStdHandle(STD_OUTPUT_HANDLE):log;
    start.hStdError=log==INVALID_HANDLE_VALUE?GetStdHandle(STD_ERROR_HANDLE):log;
    PROCESS_INFORMATION process{};bool launched=CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&start,&process)!=0;
    CloseHandle(read);if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);
    if(!launched){CloseHandle(write);editor::set_capture_frame_rate(0);std::lock_guard lock(mutex);
        current.message="Could not launch FFmpeg";current.recording=false;current.finalizing=false;close_worker=true;room.notify_all();return;}
    bool failed=false;
    for(;;){
        std::vector<unsigned char> frame;
        {
            std::unique_lock lock(mutex);ready.wait(lock,[]{return close_worker || !queue.empty();});
            if(queue.empty() && close_worker)break;
            frame=std::move(queue.front());queue.pop_front();room.notify_one();
        }
        if(!write_all(write,frame.data(),frame.size())){failed=true;break;}
        {std::lock_guard lock(mutex);++current.frames;}
    }
    CloseHandle(write);WaitForSingleObject(process.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(process.hProcess,&code);
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    editor::set_capture_frame_rate(0);
    {std::lock_guard lock(mutex);queue.clear();current.recording=false;current.finalizing=false;close_worker=true;
        if(failed||code)current.message="FFmpeg failed; see recordings/ffmpeg-last.log";
        else if(!current.message.starts_with("Recording stopped:"))
            current.message=current.frames==0?"No video frames captured; check ReShade/theater log":"Recording saved";}
    room.notify_all();
}
void finish(){
    {
        std::lock_guard lock(mutex);if(!current.recording)return;
        current.recording=false;current.finalizing=true;close_worker=true;ready.notify_all();
    }
    editor::set_capture_frame_rate(0);
    discard_staging();
}
void begin(IDXGISwapChain* swap,ID3D11Device* device,ID3D11DeviceContext* context){
    if(worker.joinable())worker.join();
    auto executable=ffmpeg_path();if(executable.empty()){
        std::lock_guard lock(mutex);current.message="FFmpeg missing: place ffmpeg.exe in MVM/Theater";return;}
    ID3D11Texture2D* backbuffer=nullptr;
    if(FAILED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&backbuffer))))return;
    D3D11_TEXTURE2D_DESC desc{};backbuffer->GetDesc(&desc);backbuffer->Release();
    if(desc.SampleDesc.Count!=1 || (desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)){
        std::lock_guard lock(mutex);current.message="Unsupported backbuffer format for capture";return;}
    expected_width=desc.Width;expected_height=desc.Height;expected_format=desc.Format;
    auto cfg=get_settings();auto file=new_filename(cfg.codec);
    {std::lock_guard lock(mutex);queue.clear();close_worker=false;post_effect_seen=false;current={true,false,false,0,
        std::filesystem::path(file).string(),post_effect_source?"Waiting for ReShade effects...":"Recording game video (ReShade not connected)"};}
    editor::set_capture_frame_rate(cfg.fixed_step?cfg.fps:0);
    worker=std::thread(encode,executable,file,desc.Width,desc.Height,desc.Format,cfg);
}
void capture_texture(ID3D11Texture2D* texture,ID3D11Device* device,ID3D11DeviceContext* context){
    if(!get_status().recording || !texture)return;
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
    if(desc.Width!=expected_width || desc.Height!=expected_height || desc.Format!=expected_format || desc.SampleDesc.Count!=1){
        finish();std::lock_guard lock(mutex);current.message="Recording stopped: ReShade output format differs from the game frame";return;
    }
    if(!staging || desc.Width!=width || desc.Height!=height || desc.Format!=format){
        if(staging){finish();std::lock_guard lock(mutex);current.message="Recording stopped: resolution or format changed";return;}
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
        if(FAILED(device->CreateTexture2D(&desc,nullptr,&staging))){finish();return;}
        width=desc.Width;height=desc.Height;format=desc.Format;
    }
    context->CopyResource(staging,texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if(FAILED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped))){finish();return;}
    std::vector<unsigned char> frame(size_t(width)*height*4);
    for(UINT y=0;y<height;++y)std::memcpy(frame.data()+size_t(y)*width*4,static_cast<const unsigned char*>(mapped.pData)+size_t(y)*mapped.RowPitch,size_t(width)*4);
    context->Unmap(staging,0);
    std::unique_lock lock(mutex);room.wait(lock,[]{return queue.size()<4 || !current.recording;});
    if(current.recording){queue.push_back(std::move(frame));ready.notify_one();}
}
}
settings get_settings(){std::lock_guard lock(mutex);return options;}
void load_settings(){
    try{std::ifstream in("MVM/Theater/recording.json");if(!in)return;nlohmann::json j;in>>j;
        settings value;value.fps=j.value("fps",60);value.codec=j.value("codec",2);value.fixed_step=j.value("fixed_step",true);
        std::lock_guard lock(mutex);options.fps=std::clamp(value.fps,1,240);options.codec=std::clamp(value.codec,0,2);options.fixed_step=value.fixed_step;
    }catch(...){}
}
void set_settings(settings value){
    {std::lock_guard lock(mutex);options.fps=std::clamp(value.fps,1,240);options.codec=std::clamp(value.codec,0,2);options.fixed_step=value.fixed_step;value=options;}
    try{std::filesystem::create_directories("MVM/Theater");auto path=std::filesystem::path("MVM/Theater/recording.json");
        auto temp=path;temp+=".tmp";{std::ofstream out(temp);out<<nlohmann::json{{"fps",value.fps},{"codec",value.codec},{"fixed_step",value.fixed_step}}.dump(2)<<'\n';}
        std::error_code ec;std::filesystem::rename(temp,path,ec);if(ec){std::filesystem::remove(path,ec);std::filesystem::rename(temp,path,ec);}
    }catch(...){}
}
status get_status(){std::lock_guard lock(mutex);auto s=current;s.fixed_step=s.recording && editor::capture_frame_lock_active();return s;}
void toggle(){std::lock_guard lock(mutex);toggle_pending=true;}
void set_post_effect_source(bool enabled){post_effect_source=enabled;}
void post_effect(ID3D11Texture2D* texture,ID3D11Device* device,ID3D11DeviceContext* context){
    if(!post_effect_source || !get_status().recording)return;
    if(!post_effect_seen.exchange(true)){std::lock_guard lock(mutex);current.message="Recording ReShade finished effects";}
    capture_texture(texture,device,context);
}
void stop(){finish();if(worker.joinable())worker.join();}
void present(IDXGISwapChain* swap,ID3D11Device* device,ID3D11DeviceContext* context,bool playing){
    bool toggle_now=false;{std::lock_guard lock(mutex);toggle_now=toggle_pending;toggle_pending=false;}
    if(toggle_now){if(get_status().recording)finish();else if(playing)begin(swap,device,context);}
    if(!playing){finish();return;}
    if(!get_status().recording || post_effect_source)return;
    ID3D11Texture2D* backbuffer=nullptr;
    if(FAILED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&backbuffer)))){finish();return;}
    capture_texture(backbuffer,device,context);backbuffer->Release();
}
void shutdown(){stop();}
}
