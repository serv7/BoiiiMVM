"""Read-only live inspection: module headers, command and Lua tables."""
import ctypes as c, struct, json, re
from ctypes import wintypes as w
from pathlib import Path
import sys
k=c.WinDLL('kernel32',use_last_error=True); ps=c.WinDLL('psapi',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD]; k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes=[w.HANDLE]
ps.EnumProcessModules.argtypes=[w.HANDLE,c.c_void_p,w.DWORD,c.POINTER(w.DWORD)]
ps.GetModuleBaseNameW.argtypes=[w.HANDLE,c.c_void_p,w.LPWSTR,w.DWORD]
h=k.OpenProcess(0x410,False,int(sys.argv[1]))
def read(addr,size):
    b=c.create_string_buffer(size); got=c.c_size_t()
    k.ReadProcessMemory(h,addr,b,size,c.byref(got)); return b.raw[:got.value]
mods=(c.c_void_p*1024)(); needed=w.DWORD()
ps.EnumProcessModules(h,mods,c.sizeof(mods),c.byref(needed))
for addr in mods[:needed.value//c.sizeof(c.c_void_p)]:
    name=c.create_unicode_buffer(260); ps.GetModuleBaseNameW(h,addr,name,260)
    if name.value.lower()=='blackops3.exe': base=addr; break
hdr=read(base,4096); nt=struct.unpack_from('<I',hdr,0x3c)[0]
sec=nt+24+struct.unpack_from('<H',hdr,nt+20)[0]
sections=[]
for i in range(struct.unpack_from('<H',hdr,nt+6)[0]):
    off=sec+i*40; name=hdr[off:off+8].rstrip(b'\0').decode(); size,rva=struct.unpack_from('<II',hdr,off+8)
    sections.append((name,size,rva))
Path('docs/live-module.json').write_text(json.dumps({'base':hex(base),'sections':sections},indent=2))
print('BASE',hex(base),'SECTIONS',sections)
names=['SwitchDemoCameraMode','SwitchDemoFreeCameraMode','IsDemoPlaying']
ptrs={}
for name,size,rva in sections:
    if name not in ['.rdata','.text'] or size>60000000: continue
    d=read(base+rva,size)
    Path('docs/live'+name+'.bin').write_bytes(d)
    if name=='.rdata':
        for s in names:
            idx=d.find(s.encode()+b'\0')
            if idx>=0: ptrs[struct.pack('<Q',base+rva+idx)]=s
for name,size,rva in sections:
    if name not in ['.data','.rdata']: continue
    for offset in range(0,size,8*1024*1024):
        d=read(base+rva+offset,min(8*1024*1024,size-offset))
        for needle,s in ptrs.items():
            start=0
            while (idx:=d.find(needle,start))>=0:
                start=idx+8; raw=d[max(0,idx-16):idx+40]
                print('LUA',s,hex(rva+offset+idx),raw.hex(),[hex(x-base) for x in struct.unpack('<'+'Q'*(len(raw)//8),raw[:len(raw)//8*8])])
for rva in [0x1681efd8,0x1689df58]:
    node=base+rva; seen=set()
    for _ in range(2200):
        if not node or node in seen: break
        seen.add(node); raw=read(node,48)
        if len(raw)!=48: break
        nxt,nameptr,_,_,fn,_=struct.unpack('<QQQQQQ',raw)
        s=read(nameptr,100).split(b'\0')[0].decode(errors='replace') if nameptr else ''
        if s in ['exec','bind','vstr','toggle','set'] or s.startswith('demo_'): print('CMD',s,hex(fn-base))
        node=nxt
k.CloseHandle(h)
