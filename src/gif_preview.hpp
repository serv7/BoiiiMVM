#pragma once
#include <d3d11.h>
#include <string>
#include <filesystem>
namespace theater::gif_preview {
void set_device(ID3D11Device* device);
void shutdown();
void set_directory(std::filesystem::path directory);
// Call after RenderDrawData to retire images referenced by this frame's commands.
void finish_frame();
bool loading();
// Nonblocking: returns null while the selected GIF's first frame is decoding.
// Returned resources remain valid until finish_frame(), even across selections.
ID3D11ShaderResourceView* frame(const std::string& name);
}
