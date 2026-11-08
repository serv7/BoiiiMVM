from pathlib import Path
code = Path('docs/live.text.bin').read_bytes()
path = Path('src/build_profile.hpp')
source = path.read_text()
for name, rva in [('timeline_bounds', 0x2649e70), ('demo_paused', 0x2611770), ('camera_movement',0x25cfe40)]:
    data = ''.join('\\x%02x' % x for x in code[rva-0x1000:rva-0x1000+16])
    line = f'inline constexpr fingerprint {name} = {{{hex(rva)}, "{data}", 16}};\n'
    if f'fingerprint {name} ' not in source:
        source = source.replace('inline constexpr uintptr_t theater_pointer', line+'inline constexpr uintptr_t theater_pointer')
path.write_text(source)
