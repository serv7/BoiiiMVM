"""Read-only: collect mvm_dump_1..N string dvars from the running game into a file.
usage: readdump.py pid out.txt [prefix]"""
import ctypes, struct, sys
from ctypes import wintypes
pid = int(sys.argv[1]); out = sys.argv[2]; prefix = sys.argv[3] if len(sys.argv) > 3 else "mvm_dump"
k32 = ctypes.WinDLL("kernel32"); psapi = ctypes.WinDLL("psapi")
k32.OpenProcess.restype = wintypes.HANDLE
k32.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.LPVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
h = k32.OpenProcess(0x0410, False, pid)
mods = (ctypes.c_void_p * 1024)(); need = wintypes.DWORD()
psapi.EnumProcessModulesEx(h, mods, ctypes.sizeof(mods), ctypes.byref(need), 3)
base = 0
for m in mods[:need.value // 8]:
    if not m: continue
    nb = ctypes.create_unicode_buffer(260); psapi.GetModuleBaseNameW(h, ctypes.c_void_p(m), nb, 260)
    if nb.value.lower() == "blackops3.exe": base = m
def rd(a, n):
    buf = ctypes.create_string_buffer(n); got = ctypes.c_size_t()
    return ctypes.string_at(buf, got.value) if k32.ReadProcessMemory(h, ctypes.c_void_p(a), buf, n, ctypes.byref(got)) else b""
def hname(s):
    r = 0x4b9ace2f
    for c in s.lower().encode() + b"\0":
        r = (16777619 * (r ^ c)) & 0xFFFFFFFF
    return r
pool = base + 397173280; count = struct.unpack("<i", rd(base + 397173196, 4))[0]
data = rd(pool, count * 160)
index = {struct.unpack_from("<I", data, i * 160)[0]: i for i in range(count)}
def get(name):
    i = index.get(hname(name))
    if i is None: return None
    e = data[i * 160:(i + 1) * 160]; typ = struct.unpack_from("<I", e, 28)[0]
    ptr = struct.unpack_from("<Q", e, 40)[0]
    if typ in (7, 8) and ptr > 0x10000: return rd(ptr, 4096).split(b"\0")[0].decode("utf-8", "replace")
    return f"<type {typ} {e[40:48].hex()}>"
def collect(prefix):
    n = get(prefix + "_count")
    if n is None: return None
    try: n = int(n)
    except ValueError: return f"count={n}"
    return "".join(get(f"{prefix}_{i}") or "<missing>" for i in range(1, n + 1))
if __name__ == "__main__":
    t = collect(prefix)
    if t is None: sys.exit("no dump dvars")
    open(out, "w", encoding="utf-8").write(t)
    print(len(t), "chars ->", out)
