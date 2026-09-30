#include "../src/capture.hpp"
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace theater::editor {
void set_capture_frame_rate(int){}
bool capture_frame_lock_active(){return false;}
}
int main(int argc,char** argv){
    auto cls=WNDCLASSW{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"BO3 capture test";
    assert(RegisterClassW(&cls));
    HWND window=CreateWindowW(cls.lpszClassName,L"",WS_OVERLAPPEDWINDOW,0,0,64,64,nullptr,nullptr,cls.hInstance,nullptr);
    assert(window);
    DXGI_SWAP_CHAIN_DESC swap_desc{};swap_desc.BufferDesc.Width=64;swap_desc.BufferDesc.Height=64;
    swap_desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;swap_desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount=1;swap_desc.OutputWindow=window;swap_desc.SampleDesc.Count=1;swap_desc.Windowed=TRUE;swap_desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;IDXGISwapChain* swap=nullptr;
    assert(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,
        &swap_desc,&swap,&device,nullptr,&context)));
    ID3D11Texture2D* buffer=nullptr;ID3D11RenderTargetView* target=nullptr;
    assert(SUCCEEDED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer))));
    assert(SUCCEEDED(device->CreateRenderTargetView(buffer,nullptr,&target)));buffer->Release();
    int codec=argc>1?std::atoi(argv[1]):0;
    bool post=argc>2;
    theater::capture::set_settings({30,codec,false});theater::capture::set_post_effect_source(post);theater::capture::toggle();
    for(int i=0;i<8;++i){float color[]={float(i)/8.f,.3f,.7f,1};context->ClearRenderTargetView(target,color);
        theater::capture::present(swap,device,context,true);
        if(post){ID3D11Texture2D* frame=nullptr;assert(SUCCEEDED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&frame))));
            theater::capture::post_effect(frame,device,context);frame->Release();}
        swap->Present(0,0);}
    theater::capture::stop();auto result=theater::capture::get_status();
    std::cout<<result.message<<"; frames="<<result.frames<<"; file="<<result.file<<'\n';
    assert(result.frames==8 && result.message=="Recording saved");
    assert(std::filesystem::file_size(result.file)>1000);
    target->Release();swap->Release();context->Release();device->Release();DestroyWindow(window);
}
