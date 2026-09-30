#pragma once
#include <d3d11.h>
#include <string>

namespace theater::capture {
struct settings {int fps=60;int codec=2;bool fixed_step=true;};
struct status {bool recording=false,finalizing=false,fixed_step=false;unsigned long long frames=0;std::string file,message;};
settings get_settings();
void load_settings();
void set_settings(settings value);
status get_status();
void toggle();
void stop();
void present(IDXGISwapChain* swap,ID3D11Device* device,ID3D11DeviceContext* context,bool playing);
void post_effect(ID3D11Texture2D* texture,ID3D11Device* device,ID3D11DeviceContext* context);
void set_post_effect_source(bool enabled);
void shutdown();
}
