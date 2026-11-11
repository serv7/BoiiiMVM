"""Post key messages to the BO3 window without focusing it.
usage: keys.py key <vk-hex> [char]   |   keys.py text "string"   |   keys.py console "command" (types and presses Enter)"""
import ctypes, sys, time
from ctypes import wintypes
sys.path.insert(0, __import__("os").path.dirname(__file__))
from grab import find_window, u32
u32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u32.MapVirtualKeyW.argtypes = [wintypes.UINT, wintypes.UINT]
u32.VkKeyScanW.argtypes = [wintypes.WCHAR]; u32.VkKeyScanW.restype = ctypes.c_short
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
hwnd = find_window()

def key(vk, ch=None, hold=0.03):
    sc = u32.MapVirtualKeyW(vk, 0)
    if vk in (0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E): sc |= 0x100  # extended keys
    u32.PostMessageW(hwnd, WM_KEYDOWN, vk, 1 | (sc << 16))
    if ch: u32.PostMessageW(hwnd, WM_CHAR, ord(ch), 1 | (sc << 16))
    time.sleep(hold)
    u32.PostMessageW(hwnd, WM_KEYUP, vk, 1 | (sc << 16) | (3 << 30))
    time.sleep(0.03)

def text(s):
    for c in s:
        vk = u32.VkKeyScanW(c) & 0xFF
        sc = u32.MapVirtualKeyW(vk, 0)
        u32.PostMessageW(hwnd, WM_CHAR, ord(c), 1 | (sc << 16))
        time.sleep(0.01)

if __name__ == "__main__":
    if not hwnd: sys.exit("no game window")
    mode = sys.argv[1]
    if mode == "key": key(int(sys.argv[2], 16), sys.argv[3] if len(sys.argv) > 3 else None)
    elif mode == "text": text(sys.argv[2])
    elif mode == "console": text(sys.argv[2]); time.sleep(0.05); key(0x0D, "\r")
    print("sent")
