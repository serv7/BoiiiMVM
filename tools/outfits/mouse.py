"""Post mouse messages to the BO3 window (client coordinates). usage: mouse.py move x y | click x y"""
import sys, time, os
sys.path.insert(0, os.path.dirname(__file__))
from keys import u32, hwnd
def move(x, y): u32.PostMessageW(hwnd, 0x200, 0, (y << 16) | x)
def click(x, y):
    move(x, y); time.sleep(0.12); u32.PostMessageW(hwnd, 0x201, 1, (y << 16) | x); time.sleep(0.08); u32.PostMessageW(hwnd, 0x202, 0, (y << 16) | x)
if __name__ == "__main__":
    x, y = int(sys.argv[2]), int(sys.argv[3])
    (click if sys.argv[1] == "click" else move)(x, y); print("ok")
