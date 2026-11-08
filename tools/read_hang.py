import struct
from pathlib import Path
d=Path('docs/f3-hang.dmp').read_bytes()
u32=lambda o:struct.unpack_from('<I',d,o)[0]
u64=lambda o:struct.unpack_from('<Q',d,o)[0]
streams={u32(u32(12)+i*12):(u32(u32(12)+i*12+8),u32(u32(12)+i*12+4)) for i in range(u32(8))}
modules=[];rva,_=streams[4]
for i in range(u32(rva)):
    o=rva+4+i*108;name_rva=u32(o+20);name=d[name_rva+4:name_rva+4+u32(name_rva)].decode('utf-16le').split('\\')[-1]
    modules.append((u64(o),u32(o+8),name))
def locate(address):
    for base,size,name in modules:
        if base<=address<base+size:return name,address-base
    return None
rva,_=streams[3]
for i in range(u32(rva)):
    o=rva+4+i*48;tid=u32(o);srva=u32(o+36);ssize=u32(o+32);crva=u32(o+44)
    rsp=u64(crva+152);rip=u64(crva+248);stack_va=u64(o+24)
    frames=[]
    for off in range(0,ssize-7,8):
        ptr=u64(srva+off);match=locate(ptr)
        if match and (match[0] in ['bo3_theater.dll','BlackOps3.exe','boiii.exe','ntdll.dll','KERNELBASE.dll','MSVCP140.dll','VCRUNTIME140.dll']):
            if match[0]=='BlackOps3.exe' and match[1]>0x3000000:continue
            frames.append((stack_va+off-rsp,*match,ptr))
    if any(f[1]=='bo3_theater.dll' for f in frames):
        print('THREAD',tid,'RIP',hex(rip),locate(rip),'RSP',hex(rsp))
        for offset,name,address,ptr in frames[:35]: print(hex(offset),name,hex(address),hex(ptr))
