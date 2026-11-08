"""Analyze the captured game code without running or patching it."""
import bisect
import json
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent / '.analysis-libs'))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

meta = json.loads(Path('loaded-demo-strings.json').read_text())
base = int(meta['base'], 16)
code = Path('loaded.text.bin').read_bytes()
pdata = Path('loaded.pdata.bin').read_bytes()
functions = []
for off in range(0,len(pdata)-11,12):
    start,end,unwind = struct.unpack_from('<III',pdata,off)
    if 0x1000 <= start < end < 0x2efe000:
        functions.append((start,end))
functions.sort()
starts = [f[0] for f in functions]
md = Cs(CS_ARCH_X86, CS_MODE_64)
def containing(rva):
    i = bisect.bisect_right(starts,rva)-1
    if i>=0 and rva < functions[i][1]:
        return functions[i]
    return rva-40,rva+80
def show(start,end):
    for ins in md.disasm(code[start-0x1000:end-0x1000],base+start):
        print(hex(ins.address-base),ins.mnemonic,__import__('re').sub(r'0x7ff[0-9a-f]+', lambda m: hex(int(m.group(),16)-base), ins.op_str))

if len(sys.argv)>1 and sys.argv[1] == 'calls':
    import re
    targets = {int(a,0) for a in sys.argv[2:]}
    for m in re.finditer(rb'[\xe8\xe9]....',code,re.S):
        rva=0x1000+m.start()
        target=rva+5+struct.unpack('<i',m.group()[1:])[0]
        if target in targets:
            start,end=containing(rva)
            print('CALL',hex(target),hex(rva),'FUNCTION',hex(start),hex(end))
            show(max(start,rva-80),min(end,rva+50))
elif len(sys.argv)>1 and sys.argv[1] == 'refs':
    import re
    targets = {int(a,0) for a in sys.argv[2:]}
    for m in re.finditer(rb'[\x40-\x4f]?[\x8b\x89\x8d\xff\x3b\x39][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]....',code,re.S):
        ins = next(md.disasm(m.group(), base+0x1000+m.start()),None)
        if not ins or len(ins.bytes) != len(m.group()):
            continue
        target=ins.address-base+ins.size+struct.unpack('<i',m.group()[-4:])[0]
        if target in targets:
            start,end=containing(ins.address-base)
            print('REF',hex(target),hex(ins.address-base),'FUNCTION',hex(start),hex(end))
            show(start,min(end,start+0x400))
elif len(sys.argv)>1:
    rva = int(sys.argv[1],0)
    start,end = containing(rva)
    if len(sys.argv)>2:
        start,end = rva,rva+int(sys.argv[2],0)
    print('FUNCTION',hex(start),hex(end))
    show(start,end)
else:
    for name,val in [('demo_enabled',0xe1f4807e),('demo_usefilesystem',0x4004e14d),
                     ('demo_recordPrivateMatch',0x3a5838a4),('demo_recordOfflineMatch',0x743a1c60),
                     ('demo_recordSystemlinkMatch',0xe5e25ab2)]:
        needle=struct.pack('<I',val)
        off=0
        while (off:=code.find(needle,off))>=0:
            rva=0x1000+off
            start,end=containing(rva)
            print('\nHASH',name,hex(rva),'FUNCTION',hex(start),hex(end))
            show(max(start,rva-80),min(end,rva+65))
            off+=4


