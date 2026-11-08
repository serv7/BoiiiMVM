"""Read a saved minidump; never attach to or modify the game."""
import ctypes as c
from ctypes import wintypes as w
import mmap
import struct
import bisect
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
f = (root / 'build/animation-crash/crash.dmp').open('rb')
d = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
u32 = lambda o: struct.unpack_from('<I', d, o)[0]
u64 = lambda o: struct.unpack_from('<Q', d, o)[0]
streams = {u32(u32(12)+i*12): (u32(u32(12)+i*12+8), u32(u32(12)+i*12+4)) for i in range(u32(8))}
modules = []
rva, _ = streams[4]
for i in range(u32(rva)):
    o = rva+4+i*108
    nr = u32(o+20)
    name = d[nr+4:nr+4+u32(nr)].decode('utf-16le').split('\\')[-1]
    modules.append((u64(o), u32(o+8), name))

dbg = c.WinDLL('C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/dbghelp.dll')
proc = c.c_void_p(-1)
dbg.SymInitializeW.argtypes = [c.c_void_p, w.LPCWSTR, w.BOOL]
dbg.SymInitializeW(proc, str(root / 'build'), False)
dbg.SymLoadModuleExW.argtypes = [c.c_void_p, c.c_void_p, w.LPCWSTR, w.LPCWSTR, c.c_ulonglong, w.DWORD, c.c_void_p, w.DWORD]
dbg.SymLoadModuleExW.restype = c.c_ulonglong
for base, size, name in modules:
    if name == 'bo3_theater.dll':
        print('PLUGIN', hex(base), hex(size))
        print('Symbols', hex(dbg.SymLoadModuleExW(proc, None, str(root/'build/bo3_theater.dll'), None, base, size, None, 0)))

class Symbol(c.Structure):
    _fields_ = [('SizeOfStruct',w.ULONG),('TypeIndex',w.ULONG),('Reserved',c.c_ulonglong*2),('Index',w.ULONG),('Size',w.ULONG),('ModBase',c.c_ulonglong),('Flags',w.ULONG),('Value',c.c_ulonglong),('Address',c.c_ulonglong),('Register',w.ULONG),('Scope',w.ULONG),('Tag',w.ULONG),('NameLen',w.ULONG),('MaxNameLen',w.ULONG),('Name',c.c_char*1)]
dbg.SymFromAddr.argtypes = [c.c_void_p,c.c_ulonglong,c.POINTER(c.c_ulonglong),c.c_void_p]
def locate(addr):
    for base, size, name in modules:
        if base <= addr < base+size:
            label = f'{name}+{addr-base:x}'
            if name == 'bo3_theater.dll':
                buf = c.create_string_buffer(c.sizeof(Symbol)+1024)
                s = Symbol.from_buffer(buf);s.SizeOfStruct=c.sizeof(Symbol);s.MaxNameLen=1024
                disp=c.c_ulonglong()
                if dbg.SymFromAddr(proc, addr, c.byref(disp), buf):
                    label += ' ' + c.string_at(c.addressof(buf)+Symbol.Name.offset, s.NameLen).decode(errors='replace')+f'+{disp.value:x}'
            return label
    return None

ranges=[]
if 9 in streams:
    mr=streams[9][0];cursor=u64(mr+8)
    for i in range(u64(mr)):
        start=u64(mr+16+i*16);size=u64(mr+24+i*16)
        ranges.append((start,start+size,cursor));cursor+=size
if 5 in streams:
    mr=streams[5][0]
    for i in range(u32(mr)):
        o=mr+4+i*16;ranges.append((u64(o),u64(o)+u32(o+8),u32(o+12)))
ranges.sort();starts=[r[0] for r in ranges]
def read(addr,size):
    i=bisect.bisect_right(starts,addr)-1
    if i>=0:
        a,b,r=ranges[i]
        if addr+size<=b:return bytes(d[r+addr-a:r+addr-a+size])
    return b''

if '--live' in sys.argv:
    kernel=c.WinDLL('kernel32',use_last_error=True)
    kernel.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];kernel.OpenProcess.restype=w.HANDLE
    kernel.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
    handle=kernel.OpenProcess(0x410,False,int(sys.argv[sys.argv.index('--live')+1]))
    assert handle,c.get_last_error()
    def read(addr,size):
        buf=c.create_string_buffer(size);count=c.c_size_t()
        kernel.ReadProcessMemory(handle,addr,buf,size,c.byref(count));return buf.raw[:count.value]

if '--globals' in sys.argv:
    CB=c.WINFUNCTYPE(w.BOOL,c.POINTER(Symbol),w.ULONG,c.c_void_p)
    def show(sp,size,unused):
        s=sp.contents;name=c.string_at(c.addressof(s)+Symbol.Name.offset,s.NameLen).decode(errors='replace')
        raw=read(s.Address,min(s.Size,64))
        print('GLOBAL',name,hex(s.Address),s.Size,raw.hex())
        if s.Size==8 and len(raw)==8:
            pointer=struct.unpack('<Q',raw)[0];print('   pointee',read(pointer,64).hex())
        if s.Size==32 and 'selected' in name and len(raw)==32:
            pointer,length,capacity=struct.unpack_from('<Q8xQQ',raw)
            print('   text',read(pointer,length) if capacity>15 else raw[:length])
        return True
    dbg.SymEnumSymbols.argtypes=[c.c_void_p,c.c_ulonglong,c.c_char_p,CB,c.c_void_p]
    plugin=next(b for b,s,n in modules if n=='bo3_theater.dll')
    for pattern in [b'*gif_preview*',b'*render_overlay*']:
        dbg.SymEnumSymbols(proc,plugin,pattern,CB(show),None)
    sys.path.insert(0,str(root/'.analysis-libs'))
    from capstone import Cs,CS_ARCH_X86,CS_MODE_64
    for ins in Cs(CS_ARCH_X86,CS_MODE_64).disasm(read(plugin+0x1ce10,0x240),0x1ce10):
        print(hex(ins.address),ins.mnemonic,ins.op_str)
    sys.exit()

ex = streams.get(6)
if ex:
    print('EXCEPTION', u32(ex[0]), hex(u32(ex[0]+8)),locate(u64(ex[0]+24)))
rva, _ = streams[3]
for i in range(u32(rva)):
    o=rva+4+i*48;tid=u32(o);srva=u32(o+36);ssize=u32(o+32);crva=u32(o+44)
    rsp=u64(crva+152);rip=u64(crva+248);stack_va=u64(o+24)
    frames=[]
    for off in range(0,ssize-7,8):
        ptr=u64(srva+off);match=locate(ptr)
        if match and (match.startswith(('bo3_theater','d3d11','dxgi','nvwgf2umx','dxgi','ReShade','BlackOps3'))):
            if match.startswith('BlackOps3') and ptr-next(b for b,s,n in modules if n=='BlackOps3.exe')>0x3000000:continue
            if stack_va+off>=rsp:frames.append((stack_va+off-rsp, match))
    print('THREAD',tid,'PC',locate(rip) or hex(rip))
    if any('bo3_theater' in v for _,v in frames) or tid==19680:
        for offset, label in frames[:45]:print(f'  {offset:04x} {label}')
