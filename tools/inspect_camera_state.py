"""Read-only native theater fields and loaded module bases for crash diagnosis."""
import ctypes as c, struct, sys
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True);ps=c.WinDLL('psapi',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes=[w.HANDLE]
ps.EnumProcessModules.argtypes=[w.HANDLE,c.c_void_p,w.DWORD,c.POINTER(w.DWORD)]
ps.GetModuleBaseNameW.argtypes=[w.HANDLE,c.c_void_p,w.LPWSTR,w.DWORD]
h=k.OpenProcess(0x410,False,int(sys.argv[1]))
def read(address,size):
    data=c.create_string_buffer(size);got=c.c_size_t()
    k.ReadProcessMemory(h,address,data,size,c.byref(got));return data.raw[:got.value]
def value(address,fmt):
    raw=read(address,struct.calcsize(fmt))
    return struct.unpack(fmt,raw) if len(raw)==struct.calcsize(fmt) else None
mods=(c.c_void_p*1024)();needed=w.DWORD();ps.EnumProcessModules(h,mods,c.sizeof(mods),c.byref(needed))
base=0
for address in mods[:needed.value//c.sizeof(c.c_void_p)]:
    name=c.create_unicode_buffer(260);ps.GetModuleBaseNameW(h,address,name,260)
    if name.value.lower() in ['boiii.exe','blackops3.exe','bo3_theater.dll']:
        print(name.value,hex(address))
    if name.value.lower()=='blackops3.exe':base=address
if base:
    session=value(base+0x197eba00,'Q')
    print('session',session,'tick',value(base+0x3448598,'i'),'FOV',value(base+0x2fa7cc8,'d'))
    if session and session[0]:
        for name,off,fmt in [('view',0x7dae60,'i'),('free mode',0x7dae64,'i'),('count',0x9845d8,'i'),('eye',0x981a68,'3f'),('angles',0x981a74,'3f')]:
            print(name,value(session[0]+off,fmt))
k.CloseHandle(h)
