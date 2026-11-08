#include "gif_preview.hpp"
#include <Windows.h>
#include <gdiplus.h>
#include <wrl/client.h>
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#pragma comment(lib,"gdiplus.lib")

namespace theater::gif_preview {
namespace {
using Microsoft::WRL::ComPtr;
struct decoded_frame {
    uint64_t epoch=0;UINT index=0,width=0,height=0;
    std::vector<unsigned char> pixels;
};
struct decode_request {
    std::filesystem::path path;uint64_t epoch=0,serial=0;ULONGLONG elapsed=0;
};
ComPtr<ID3D11Device> device;
ComPtr<ID3D11ShaderResourceView> view;
// ImGui stores raw SRV pointers until RenderDrawData. Changing selection must
// never release an image which an earlier draw command still references.
std::vector<ComPtr<ID3D11ShaderResourceView>> drawing;
std::filesystem::path directory="MVM/Theater/death-previews";
std::string selected;
ULONGLONG start=0;
uint64_t epoch=0;
std::mutex guard;
std::condition_variable wake;
decode_request requested;
std::shared_ptr<const decoded_frame> decoded,uploaded;
bool stopping=false;
uint64_t failed_epoch=0;
std::thread worker;

void decode_loop() {
    // GDI+ and file IO stay on this worker. It never obtains a D3D device or
    // immediate context, and never calls game functions.
    ULONG_PTR token=0;Gdiplus::GdiplusStartupInput input;
    if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok)return;
    {
        std::unique_ptr<Gdiplus::Image> image;
        uint64_t seen=0,loaded=0;UINT last=UINT_MAX,total=0;
        std::vector<UINT> delays;
        for(;;) {
            decode_request task;
            {
                std::unique_lock lock(guard);
                wake.wait(lock,[&]{return stopping || requested.serial!=seen;});
                if(stopping)break;
                task=requested;seen=task.serial;
            }
            try {
                if(task.epoch!=loaded) {
                    loaded=task.epoch;image.reset();delays.clear();total=0;last=UINT_MAX;
                    if(task.path.empty() || !std::filesystem::is_regular_file(task.path)){
                        std::lock_guard lock(guard);failed_epoch=task.epoch;continue;
                    }
                    image=std::make_unique<Gdiplus::Image>(task.path.c_str());
                    if(image->GetLastStatus()!=Gdiplus::Ok || !image->GetWidth() || !image->GetHeight() ||
                        image->GetWidth()>1024 || image->GetHeight()>1024){image.reset();std::lock_guard lock(guard);failed_epoch=task.epoch;continue;}
                    UINT count=image->GetFrameCount(&Gdiplus::FrameDimensionTime);
                    if(!count || count>2000){image.reset();std::lock_guard lock(guard);failed_epoch=task.epoch;continue;}
                    delays.assign(count,100);
                    UINT bytes=image->GetPropertyItemSize(PropertyTagFrameDelay);
                    if(bytes>=sizeof(Gdiplus::PropertyItem) && bytes<65536) {
                        std::vector<unsigned char> data(bytes);
                        auto item=reinterpret_cast<Gdiplus::PropertyItem*>(data.data());
                        if(image->GetPropertyItem(PropertyTagFrameDelay,bytes,item)==Gdiplus::Ok &&
                            item->value && item->length>=count*sizeof(UINT))
                            for(UINT i=0;i<count;++i)delays[i]=std::clamp(static_cast<UINT*>(item->value)[i],2u,1000u)*10;
                    }
                    for(auto delay:delays)total+=delay;
                }
                if(!image || !total)continue;
                UINT remaining=UINT(task.elapsed%total),index=0;
                while(index+1<delays.size() && remaining>=delays[index])remaining-=delays[index++];
                if(index==last)continue;
                if(image->SelectActiveFrame(&Gdiplus::FrameDimensionTime,index)!=Gdiplus::Ok)continue;
                auto result=std::make_shared<decoded_frame>();result->epoch=task.epoch;result->index=index;
                result->width=image->GetWidth();result->height=image->GetHeight();
                Gdiplus::Bitmap pixels(result->width,result->height,PixelFormat32bppARGB);
                {
                    Gdiplus::Graphics graphics(&pixels);graphics.Clear(Gdiplus::Color(0,0,0,0));
                    if(graphics.DrawImage(image.get(),0,0,int(result->width),int(result->height))!=Gdiplus::Ok)continue;
                }
                Gdiplus::BitmapData data{};Gdiplus::Rect rectangle(0,0,result->width,result->height);
                if(pixels.LockBits(&rectangle,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data)!=Gdiplus::Ok)continue;
                result->pixels.resize(size_t(result->width)*result->height*4);
                for(UINT y=0;y<result->height;++y)
                    std::memcpy(result->pixels.data()+size_t(y)*result->width*4,
                        static_cast<unsigned char*>(data.Scan0)+ptrdiff_t(y)*data.Stride,result->width*4);
                pixels.UnlockBits(&data);last=index;
                std::lock_guard lock(guard);
                if(requested.epoch==task.epoch)decoded=std::move(result);
            } catch(...) {image.reset();std::lock_guard lock(guard);failed_epoch=task.epoch;}
        }
    }
    Gdiplus::GdiplusShutdown(token);
}

void invalidate() {
    view.Reset();uploaded.reset();selected.clear();++epoch;
    {std::lock_guard lock(guard);auto serial=requested.serial+1;decoded.reset();requested={};requested.epoch=epoch;requested.serial=serial;}
    wake.notify_one();
}
}

void set_device(ID3D11Device* value) {if(device.Get()!=value){invalidate();device=value;}}
void set_directory(std::filesystem::path path) {invalidate();directory=std::move(path);}
void finish_frame(){drawing.clear();}
bool loading(){std::lock_guard lock(guard);return !uploaded && failed_epoch!=epoch;}
void shutdown() {
    {std::lock_guard lock(guard);stopping=true;}
    wake.notify_one();if(worker.joinable())worker.join();
    drawing.clear();view.Reset();device.Reset();uploaded.reset();selected.clear();
    {std::lock_guard lock(guard);decoded.reset();requested={};stopping=false;}
}

ID3D11ShaderResourceView* frame(const std::string& name) {
    if(!device || name.empty())return nullptr;
    // Catalog names are identifiers, never paths supplied to the decoder.
    if(name.find_first_of("/\\:")!=std::string::npos || name=="." || name=="..")return nullptr;
    if(!worker.joinable())worker=std::thread(decode_loop);
    bool changed=name!=selected;
    if(changed){selected=name;view.Reset();uploaded.reset();++epoch;start=GetTickCount64();}
    std::shared_ptr<const decoded_frame> ready;
    {
        std::lock_guard lock(guard);
        requested.path=directory/(name+".gif");requested.epoch=epoch;
        requested.elapsed=GetTickCount64()-start;++requested.serial;
        ready=decoded;
    }
    wake.notify_one();
    if(ready && ready->epoch==epoch && ready!=uploaded) {
        D3D11_TEXTURE2D_DESC desc{};desc.Width=ready->width;desc.Height=ready->height;
        desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;
        // Immutable images avoid Map/Unmap and resource renaming on BO3's
        // immediate context (also wrapped by ReShade).
        desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=ready->pixels.data();data.SysMemPitch=ready->width*4;
        ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> next;
        if(SUCCEEDED(device->CreateTexture2D(&desc,&data,&texture)) &&
            SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,&next))){view=std::move(next);uploaded=std::move(ready);}
    }
    if(view)drawing.push_back(view);
    return view.Get();
}
}
