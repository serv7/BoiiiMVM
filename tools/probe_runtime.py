"""Read-only BO3 runtime inspection. No process memory is modified."""
import ctypes as c, struct, sys, json
from ctypes import wintypes as w
from pathlib import Path
k=c.WinDLL('kernel32',use_last_error=True);ps=c.WinDLL('psapi',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
ps.EnumProcessModules.argtypes=[w.HANDLE,c.c_void_p,w.DWORD,c.POINTER(w.DWORD)]
ps.GetModuleBaseNameW.argtypes=[w.HANDLE,c.c_void_p,w.LPWSTR,w.DWORD]
h=k.OpenProcess(0x410,False,int(sys.argv[1]));assert h, c.get_last_error()
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t();k.ReadProcessMemory(h,a,b,n,c.byref(got));return b.raw[:got.value]
def unpack(fmt,a):
 b=read(a,struct.calcsize(fmt));return struct.unpack(fmt,b) if len(b)==struct.calcsize(fmt) else ()
def ptr(a):
 v=unpack('<Q',a);return v[0] if v else 0
def text(a):return read(a,256).split(b'\0')[0].decode(errors='replace')
mods=(c.c_void_p*1024)();needed=w.DWORD();ps.EnumProcessModules(h,mods,c.sizeof(mods),c.byref(needed))
modules={}
for a in mods[:needed.value//c.sizeof(c.c_void_p)]:
 n=c.create_unicode_buffer(260);ps.GetModuleBaseNameW(h,a,n,260);modules[n.value]=a
base=next(v for n,v in modules.items() if n.lower()=='blackops3.exe')
print('BASE',hex(base),'PLUGIN',[(n,hex(a)) for n,a in modules.items() if 'theater' in n.lower() or 'reshade' in n.lower()])
session=ptr(base+0x197eba00)
print('SESSION',hex(session),'tick',unpack('<i',base+0x3448598),'view/mode',unpack('<ii',session+0x7dae60),'speed',unpack('<f',session+8),'catchers',unpack('<I',base+0x53d8bc4))
print('camera',unpack('<9f',session+0x981a68))
print('pool3',unpack('<QII?3xIQ',base+0x94073f0+3*32))
result=[]
for kind in range(6):
 values=unpack('<QII?3xIQ',base+0x94073f0+kind*32)
 print('pool',kind,values)
 if kind!=3 or not values:continue
 if '--headers' in sys.argv:
  for i in range(12):
   b=read(values[0]+i*values[1],values[1]);names=[]
   for off in range(0,len(b)-7,8):
    pointer=struct.unpack_from('<Q',b,off)[0]
    if 0x100000000<pointer<0x7fffffffffff:
     candidate=text(pointer)
     if len(candidate)>4 and all(31<ord(ch)<127 for ch in candidate): names.append((hex(off),candidate))
   print('HEADER',i,b.hex(),names)

 pool,size,count,single,used,free=values
 if not 16<=size<=2048 or not 0<count<100000:continue
 for i in range(count):
  a=pool+i*size;b=read(a,size)
  if len(b)<0xf8:continue
  nameptr=struct.unpack_from('<Q',b,0)[0]
  name=text(nameptr)
  if name and all(31<ord(ch)<127 for ch in name):
   fps=struct.unpack_from('<f',b,0x50)[0];frames,bones=struct.unpack_from('<HH',b,0x20)
   result.append({'address':hex(a),'name':name,'fps':fps,'bones':bones,'frames':frames,'record':b.hex()})
 print('Named pool slots',len(result),'death',sum('death' in item['name'].lower() for item in result))
 print('SAMPLES',[(x['name'],x['fps'],x['bones'],x['frames']) for x in result[:8]])
Path('build/runtime-assets.json').write_text(json.dumps({'base':hex(base),'session':hex(session),'animations':result},indent=2))
if '--active' in sys.argv:
 handles=unpack('<1794H',base+0x16dd3540)
 names={int(x['address'],16):x['name'] for x in result}
 for entity,index in enumerate(handles):
  if not index:continue
  tree=ptr(base+0x16cf4930+index*0x128)
  root=unpack('<I',tree+8)
  if not root or not root[0] or root[0]>0x10000:continue
  stack=[root[0]];seen=set();active=[]
  while stack and len(seen)<4096:
   node=stack.pop()
   if node in seen or not 0<node<0x10000:continue
   seen.add(node);b=read(base+0x17e77110+node*0x58,0x58)
   if len(b)!=0x58:continue
   sibling,child=struct.unpack_from('<II',b,0x44)
   if sibling:stack.append(sibling)
   if child:stack.append(child)
   parts=struct.unpack_from('<Q',b,8)[0]
   if parts in names:
    name=names[parts]
    if 'death' in name:active.append((node,name,b.hex()))
  if entity<30 or active: print('ACTIVE',entity,'root',root[0],'nodes',len(seen),active)
if '--methods' in sys.argv:
 import re
 header=Path(r'C:/Users/servg/Documents/ChatGPT/bo3 demos/boiii-free/src/client/game/structs/scr/builtin/table/cscr.hpp').read_text()
 names=re.findall(r'BuiltinMethodDef (\w+);',header[header.index('union BuiltinMethodTable'):])
 for name in ['GetAnimTime','AttachWeapon','GetPlayerCorpse','GetTagOrigin']:
  index=names.index(name);function=ptr(base+0x32e3610+index*32+16)
  print('METHOD',name,index,hex(function-base));Path('build/native-'+name+'.bin').write_bytes(read(function,0x2000))
if '--dvars' in sys.argv:
 def hashname(name):
  value=0x4b9ace2f
  for ch in name.lower().encode()+b'\0':value=((value^ch)*0x1000193)&0xffffffff
  return value
 names={hashname(x):x for x in ['com_maxfps','com_fixedtime','com_fixedtime_float','r_vsync','timescale','demo_timescale','mvm_holdgun','mvm_weapon_drops','mvm_releasegun']}
 count=unpack('<i',base+397173196)[0];data=read(base+397173280,count*160)
 for off in range(0,len(data),160):
  key=struct.unpack_from('<I',data,off)[0]
  if key in names:
   print('DVAR',names[key],hex(397173280+off),'type',struct.unpack_from('<I',data,off+28)[0], 'raw current',data[off+40:off+56].hex(), 'float',struct.unpack_from('<f',data,off+40)[0])
if '--dobj' in sys.argv:
 handles=unpack('<1794H',base+0x16dd3540)
 for entity,index in enumerate(handles):
  if not index:continue
  dobj=base+0x16cf4930+index*0x128;b=read(dobj,0x128)
  print('DOBJ',entity,index,hex(dobj),b[:96].hex())
  for off in range(0,64,8):
   p=ptr(dobj+off)
   if p>0x100000000:
    data=read(p,64)
    if len(data)==64:print(' pointer',hex(off),hex(p),data.hex(),'nested',read(ptr(p),48).hex())
  if entity>150:break
k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle(h)
