#pragma once
#include <d3d11.h>
#include <cstdint>
namespace theater::outfit_menu {
// Replaces the body of the bot menu's Specialist page (drawn by boiii.exe) with one that lists
// outfits by name and shows a picture of the one under the mouse. Does nothing unless the exact
// client build is recognised; the stock page stays in that case.
bool initialize(uintptr_t host,void(*log)(const char*));
void shutdown();
void set_device(ID3D11Device* device);
}
