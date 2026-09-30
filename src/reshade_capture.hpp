#pragma once
namespace theater::reshade_capture {
void initialize();
void shutdown();
bool active();
bool frame_seen();
void begin_present();
void mark_pre_render();
}
