"""Generate byte fingerprints and locate native functions in the captured build."""
import sys, json, struct, re, bisect
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.analysis-libs'))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
p=Path(r'C:\Users\servg\Documents\ChatGPT\bo3 demos')
code=Path('docs/live.text.bin').read_bytes()
md=Cs(CS_ARCH_X86,CS_MODE_64)
pdata=(p/'loaded.pdata.bin').read_bytes()
funcs=sorted(struct.unpack_from('<III',pdata,i)[:2] for i in range(0,len(pdata)-11,12))
starts=[a for a,b in funcs]
def show(start,n=160):
    print('FUNCTION',hex(start))
    for ins in md.disasm(code[start-0x1000:start-0x1000+n],start): print(hex(ins.address),ins.mnemonic,ins.op_str)
for match in re.finditer(rb'\xc7\x80\x7c\x1a\x98\x00\x00\x00\x00\x00',code):
    rva=match.start()+0x1000; i=bisect.bisect_right(starts,rva)-1
    print('ROLL RESET',hex(rva),'FUNCTION',hex(funcs[i][0])); show(funcs[i][0],140); show(rva-30,60)
# Functions confirmed from registered native commands and their direct callees.
native={'set_timescale':0x262aa40,'seek_relative':0x2611940,
        'toggle_demo_hud':0x2638de0,'toggle_game_hud':0x2638e00,
        'demo_playing':0x2600210,'add_marker_cmd':0x25fbd20,
        'reposition_marker_cmd':0x2604b70,'remove_marker_cmd':0x2604590,
        'apply_marker_cmd':0x25ff210,'switch_view':0x25e99e0,
        'switch_free_mode':0x25eb3b0,'execute_command':0x20ed380,
        'camera_frame':0x25ce540,'jump_marker':0x25db4a0,
        'timeline_bounds':0x2649e70,'demo_paused':0x2611770,'camera_movement':0x25cfe40}
out=['#pragma once','#include <cstdint>','namespace profile {',
     'struct fingerprint { uintptr_t rva; const char* bytes; size_t length; };']
for name,rva in native.items():
    data=code[rva-0x1000:rva-0x1000+16]
    out.append('inline constexpr fingerprint '+name+' = {'+hex(rva)+', "'+''.join('\\x%02x'%x for x in data)+'", 16};')
out.extend(['inline constexpr uintptr_t theater_pointer = 0x197eba00;',
            'inline constexpr uintptr_t demo_state = 0x191df098;',
            'inline constexpr uintptr_t main_frame = 0x20f8e00;',
            'inline constexpr uintptr_t tick = 0x3448598;',
            'inline constexpr uintptr_t fov = 0x2fa7cc8;',
            'inline constexpr uintptr_t block_input = 0x162e2280;',
            'inline constexpr uintptr_t camera_pointer = 0x17b0e390;',
            'inline constexpr uintptr_t roll_reset = 0x25cfd7b;',
            '}'])
Path('src/build_profile.hpp').write_text('\n'.join(out)+'\n')
