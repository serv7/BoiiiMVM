import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.analysis-libs'))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
md=Cs(CS_ARCH_X86,CS_MODE_64)
code=Path('docs/live.text.bin').read_bytes()
for arg in sys.argv[1:]:
    rva=int(arg,0)
    print('FUNCTION',hex(rva))
    for ins in md.disasm(code[rva-0x1000:rva-0x1000+240],rva):
        print(hex(ins.address),ins.mnemonic,ins.op_str)
