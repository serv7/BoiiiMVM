"""Capture the BO3 window with PrintWindow (works while the window is covered). Saves BMP.
usage: grab.py out.bmp [scale_divisor]"""
import ctypes, sys, struct
from ctypes import wintypes
u32 = ctypes.WinDLL("user32"); g32 = ctypes.WinDLL("gdi32")
u32.SetProcessDPIAware()
u32.FindWindowW.restype = wintypes.HWND
u32.GetDC.restype = wintypes.HDC; u32.GetDC.argtypes = [wintypes.HWND]
g32.CreateCompatibleDC.restype = wintypes.HDC; g32.CreateCompatibleDC.argtypes = [wintypes.HDC]
g32.CreateCompatibleBitmap.restype = wintypes.HBITMAP; g32.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]; g32.SelectObject.restype = wintypes.HGDIOBJ
u32.PrintWindow.argtypes = [wintypes.HWND, wintypes.HDC, wintypes.UINT]
g32.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
g32.DeleteObject.argtypes = [wintypes.HGDIOBJ]; g32.DeleteDC.argtypes = [wintypes.HDC]
u32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]

def find_window():
    found = []
    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def cb(h, l):
        c = ctypes.create_unicode_buffer(64); u32.GetClassNameW(h, c, 64)
        if c.value == "CoDBlackOps" and u32.IsWindowVisible(h): found.append(h)
        return True
    u32.EnumWindows(cb, 0)
    return found[0] if found else None

def grab(hwnd):
    r = wintypes.RECT(); u32.GetClientRect(hwnd, ctypes.byref(r)); w, h = r.right, r.bottom
    dc = u32.GetDC(hwnd); mem = g32.CreateCompatibleDC(dc); bmp = g32.CreateCompatibleBitmap(dc, w, h)
    old = g32.SelectObject(mem, bmp)
    ok = u32.PrintWindow(hwnd, mem, 3)  # PW_CLIENTONLY | PW_RENDERFULLCONTENT
    bi = struct.pack("<IiiHHIIiiII", 40, w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mem, bmp, 0, h, buf, bi, 0)
    g32.SelectObject(mem, old); g32.DeleteObject(bmp); g32.DeleteDC(mem); u32.ReleaseDC(hwnd, dc)
    return ok, w, h, buf.raw

def save_bmp(path, w, h, bgra, div=1):
    if div > 1:
        rows = [bgra[y * w * 4:(y + 1) * w * 4] for y in range(0, h - h % div, div)]
        rows = [b"".join(row[x * 4:x * 4 + 4] for x in range(0, w - w % div, div)) for row in rows]
        w, h = w // div, h // div; bgra = b"".join(rows)
    hdr = struct.pack("<2sIHHI", b"BM", 54 + len(bgra), 0, 0, 54) + struct.pack("<IiiHHIIiiII", 40, w, -h, 1, 32, 0, len(bgra), 0, 0, 0, 0)
    open(path, "wb").write(hdr + bgra)

if __name__ == "__main__":
    hwnd = find_window()
    if not hwnd: sys.exit("no game window")
    ok, w, h, px = grab(hwnd)
    save_bmp(sys.argv[1], w, h, px, int(sys.argv[2]) if len(sys.argv) > 2 else 1)
    nonblack = sum(1 for i in range(0, len(px), 40004) if px[i:i + 3] != b"\0\0\0")
    print("ok", ok, w, h, "nonblack samples", nonblack)
