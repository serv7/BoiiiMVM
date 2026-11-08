"""Read-only locate the client's existing native-call proxy and TLS accessor."""
exec(open('tools/inspect_camera_state.py').read().replace('k.CloseHandle(h)', ''))
from pathlib import Path
sys.path.insert(0,str(Path('.analysis-libs').resolve()))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
md=Cs(CS_ARCH_X86,CS_MODE_64)
class MBI(c.Structure):
    _fields_=[('BaseAddress',c.c_void_p),('AllocationBase',c.c_void_p),('AllocationProtect',w.DWORD),('PartitionId',w.WORD),('RegionSize',c.c_size_t),('State',w.DWORD),('Protect',w.DWORD),('Type',w.DWORD)]
k.VirtualQueryEx.argtypes=[w.HANDLE,c.c_void_p,c.POINTER(MBI),c.c_size_t]
address=0
needle=struct.pack('<Q',base+0x1056)
prefix=bytes.fromhex('50505152535455565741504151415241534154415541564157')
while address<0x7fffffffffff:
    m=MBI()
    if not k.VirtualQueryEx(h,address,c.byref(m),c.sizeof(m)):break
    address=(m.BaseAddress or 0)+m.RegionSize
    if m.State!=0x1000 or m.Type!=0x20000 or not m.Protect&0xf0 or m.Protect&0x100:continue
    d=read(m.BaseAddress,m.RegionSize)
    start=0
    while (i:=d.find(needle,start))>=0:
        start=i+8
        begin=d.rfind(prefix,max(0,i-256),i)
        if begin<0:continue
        print('PROXY',hex(m.BaseAddress+begin))
        for ins in md.disasm(d[begin:begin+300],m.BaseAddress+begin):
            print(hex(ins.address),ins.mnemonic,ins.op_str)
        Path('docs/live-proxy.bin').write_bytes(d[begin:begin+300])
        getter=struct.unpack_from('<Q',d,begin+0xd0)[0]
        store=struct.unpack_from('<Q',d,begin+0xd8)[0]
        print('GETTER',hex(getter),'STORE',hex(store))
        for ins in md.disasm(read(getter-64,200),getter-64):
            print(hex(ins.address),ins.mnemonic,ins.op_str)
print('PLACEHOLDER',read(base+0x1054,16).hex())
k.CloseHandle(h)
