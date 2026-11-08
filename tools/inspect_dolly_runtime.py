"""Inspect a saved decrypted BO3 code slice without launching the game."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.analysis-libs'))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

code = Path('docs/live.text.bin').read_bytes()
start = int(sys.argv[1], 0)
end = int(sys.argv[2], 0)
needle = sys.argv[3].lower() if len(sys.argv) > 3 else ''
md = Cs(CS_ARCH_X86, CS_MODE_64)
for ins in md.disasm(code[start - 0x1000:end - 0x1000], start):
    line = f'{ins.address:#x} {ins.mnemonic} {ins.op_str}'
    if not needle or needle in line.lower():
        print(line)
