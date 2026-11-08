"""Offline analysis of the user's previously captured game sections."""
import json, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.analysis-libs'))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
p = Path(r'C:\Users\servg\Documents\ChatGPT\bo3 demos')
base = int(json.loads((p/'loaded-demo-strings.json').read_text())['base'],16)
text = (p/'loaded.text.bin').read_bytes()
rdata = (p/'loaded.rdata.bin').read_bytes()
pe = Path(r'E:\SteamLibrary\steamapps\common\Call of Duty Black Ops III\BlackOps3.exe').read_bytes()[:4096]
nt = struct.unpack_from('<I',pe,0x3c)[0]
sec = nt+24+struct.unpack_from('<H',pe,nt+20)[0]
for i in range(struct.unpack_from('<H',pe,nt+6)[0]):
    off=sec+40*i
    name=pe[off:off+8].strip(b'\0')
    if name==b'.rdata': rva=struct.unpack_from('<I',pe,off+12)[0]
for entry in json.loads((p/'loaded-demo-strings.json').read_text())['strings']:
    pos=rdata.find(entry['text'].encode()+b'\0')
    if pos>=0:
        rva=int(entry['rva'],16)-pos
        break
md=Cs(CS_ARCH_X86,CS_MODE_64)
print('rdata rva',hex(rva))
def show(addr,n=160):
    print('FUNCTION',hex(addr))
    for ins in md.disasm(text[addr-0x1000:addr-0x1000+n],addr):
        print(hex(ins.address),ins.mnemonic,ins.op_str)
for name in ['SwitchDemoCameraMode','SwitchDemoFreeCameraMode','GetDemoCameraMode','IsDemoPlaying','GetDemoCurrentTime']:
    pos=rdata.find(name.encode()+b'\0')
    if pos<0: print(name,'missing'); continue
    print('STRING',name,hex(rva+pos))
    ptr=struct.pack('<Q',base+rva+pos)
    start=0
    while (off:=rdata.find(ptr,start))>=0:
        start=off+8
        fn=struct.unpack_from('<Q',rdata,off+8)[0]-base
        if 0x1000<=fn<len(text)+0x1000:
            print(name,hex(fn)); show(fn,96)
import re, bisect
pdata=(p/'loaded.pdata.bin').read_bytes()
funcs=sorted(struct.unpack_from('<III',pdata,i)[:2] for i in range(0,len(pdata)-11,12))
starts=[x[0] for x in funcs]
if '--refs' in sys.argv:
    targets={rva+m.start():m.group()[:-1].decode(errors='replace') for m in re.finditer(rb'[^\0]{0,80}(?:Cbuf_AddText|SwitchDemoCameraMode|SwitchDemoFreeCameraMode)[^\0]{0,80}\0',rdata)}
    for insmatch in re.finditer(rb'[\x40-\x4f]?[\x8b\x89\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]....',text,re.S):
        ins=next(md.disasm(insmatch.group(),0x1000+insmatch.start()),None)
        if not ins or ins.size!=len(insmatch.group()): continue
        target=ins.address+ins.size+struct.unpack('<i',insmatch.group()[-4:])[0]
        if target in targets:
            i=bisect.bisect_right(starts,ins.address)-1
            print('REF',targets[target],hex(ins.address),hex(funcs[i][0])); show(funcs[i][0],min(funcs[i][1]-funcs[i][0],400))
else:
    for a in sys.argv[1:]: show(int(a,0),240)
