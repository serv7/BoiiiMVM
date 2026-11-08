"""Capture thread stacks from the user's open game without changing its files."""
import ctypes as c,sys
from ctypes import wintypes as w
from pathlib import Path
k=c.WinDLL('kernel32',use_last_error=True);d=c.WinDLL('dbghelp',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.CreateFileW.argtypes=[w.LPCWSTR,w.DWORD,w.DWORD,c.c_void_p,w.DWORD,w.DWORD,w.HANDLE];k.CreateFileW.restype=w.HANDLE
k.CloseHandle.argtypes=[w.HANDLE]
d.MiniDumpWriteDump.argtypes=[w.HANDLE,w.DWORD,w.HANDLE,w.DWORD,c.c_void_p,c.c_void_p,c.c_void_p];d.MiniDumpWriteDump.restype=w.BOOL
pid=int(sys.argv[1]);path=Path('docs/f3-hang.dmp').resolve()
h=k.OpenProcess(0x410,False,pid)
if not h:raise c.WinError(c.get_last_error())
f=k.CreateFileW(str(path),0x40000000,0,None,2,0x80,None)
if not d.MiniDumpWriteDump(h,pid,f,0x1000,None,None,None):raise c.WinError(c.get_last_error())
k.CloseHandle(f);k.CloseHandle(h);print(path,'bytes',path.stat().st_size)
