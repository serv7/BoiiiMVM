#pragma once
#include <cstdint>
namespace profile {
struct fingerprint { uintptr_t rva; const char* bytes; size_t length; };
inline constexpr fingerprint set_timescale = {0x262aa40, "\x80\x3d\xc1\x0f\x1c\x17\x00\x48\x8b\x05\xb2\x0f\x1c\x17\x74\x12", 16};
inline constexpr fingerprint seek_relative = {0x2611940, "\x40\x53\x48\x83\xec\x20\x8b\xd9\xe8\xc3\xe8\xfe\xff\x84\xc0\x74", 16};
inline constexpr fingerprint toggle_demo_hud = {0x2638de0, "\x48\x8b\x0d\x19\x2c\x1b\x17\x80\xb9\xbe\xab\x68\x00\x00\x0f\x94", 16};
inline constexpr fingerprint toggle_game_hud = {0x2638e00, "\x40\x53\x48\x83\xec\x20\x48\x8b\x05\xf3\x2b\x1b\x17\x80\xb8\xbd", 16};
inline constexpr fingerprint demo_playing = {0x2600210, "\x83\x3d\x81\xee\xbd\x16\x02\x0f\x94\xc0\xc3\xcc\x81\xf1\x2f\xce", 16};
inline constexpr fingerprint add_marker_cmd = {0x25fbd20, "\x40\x55\x48\x8b\xec\x48\x83\xec\x50\x48\x8b\x05\xf0\x1c\x07\x01", 16};
inline constexpr fingerprint reposition_marker_cmd = {0x2604b70, "\x40\x55\x48\x8b\xec\x48\x83\xec\x20\xe8\x32\xec\xb7\xff\x48\x8b", 16};
inline constexpr fingerprint remove_marker_cmd = {0x2604590, "\x48\x83\xec\x28\xe8\x17\xf2\xb7\xff\x48\x8b\x48\x18\x48\x63\x01", 16};
inline constexpr fingerprint apply_marker_cmd = {0x25ff210, "\x48\x83\xec\x28\xe8\x97\x45\xb8\xff\x48\x8b\x48\x18\x48\x63\x01", 16};
inline constexpr fingerprint switch_view = {0x25e99e0, "\x40\x55\x53\x56\x41\x56\x41\x57\x48\x8b\xec\x48\x83\xec\x20\x48", 16};
inline constexpr fingerprint switch_free_mode = {0x25eb3b0, "\x40\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8b\xec\x48\x83\xec\x20", 16};
inline constexpr fingerprint execute_command = {0x20ed380, "\x48\x83\xec\x38\xc6\x44\x24\x20\x00\xe8\x12\x00\x00\x00\x48\x83", 16};
inline constexpr fingerprint camera_frame = {0x25ce540, "\x48\x89\x5c\x24\x18\x48\x89\x7c\x24\x20\x55\x48\x8b\xec\x48\x83", 16};
inline constexpr fingerprint camera_angles_transfer = {0x22ab140, "\x48\x83\xec\x48\x0f\x28\x05\xe5\x03\x81\x15\x44\x0f\x29\x54\x24", 16};
inline constexpr fingerprint jump_marker = {0x25db4a0, "\x40\x55\x56\x48\x8b\xec\x48\x83\xec\x28\x48\x63\xf1\x48\x8b\x0d", 16};
inline constexpr fingerprint timeline_bounds = {0x2649e70, "\x8b\x05\x82\xc8\xe0\x16\x89\x01\x8b\x05\x7e\xc8\xe0\x16\x89\x02", 16};
inline constexpr fingerprint demo_paused = {0x2611770, "\x48\x83\xec\x28\xe8\x97\xea\xfe\xff\x84\xc0\x74\x2e\x48\x83\x3d", 16};
inline constexpr fingerprint camera_movement = {0x25cfe40, "\x48\x89\x7c\x24\x20\x55\x48\x8d\x6c\x24\xc0\x48\x81\xec\x40\x01", 16};
inline constexpr fingerprint camera_vertical_up = {0x25d34d2,"\x8b\x4f\x04\x8b\xc1\xc1\xe8\x11\xa8\x01",10};
inline constexpr fingerprint camera_vertical_down = {0x25d34e6,"\xc1\xe9\x10\xf6\xc1\x01",6};
inline constexpr fingerprint camera_vertical_controller = {0x25d348a,"\x66\x83\x7f\x4c\x00",5};
inline constexpr uintptr_t theater_pointer = 0x197eba00;
inline constexpr uintptr_t demo_state = 0x191df098;
inline constexpr uintptr_t main_frame = 0x20f8e00;
inline constexpr uintptr_t tick = 0x3448598;
inline constexpr uintptr_t fov = 0x2fa7cc8;
inline constexpr uintptr_t block_input = 0x162e2280;
inline constexpr uintptr_t camera_pointer = 0x17b0e390;
inline constexpr uintptr_t roll_reset = 0x25cfd7b;
}
