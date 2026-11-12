"""Writes botmod/botmod.gsc into boiii.exe in place of its built-in copy.

boiii.exe carries botmod.gsc as a fixed-size resource and rewrites
boiii/custom_scripts/mp/botmod.gsc from it on every launch, so editing the
installed file alone does nothing. The resource can't grow: comment lines are
stripped and the rest is padded with spaces to the original length.
usage: embed_botmod.py <boiii.exe>"""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MARK = b"// Black Ops 3 Bot Mod - match side"

def compact(source: str) -> bytes:
    lines = []
    for line in source.replace("\r\n", "\n").split("\n"):
        if line.strip().startswith("//"):
            continue
        if not line.strip() and lines and not lines[-1].strip():
            continue
        lines.append(line.rstrip())
    return ("// Black Ops 3 Bot Mod - match side (BoiiiMVM build)\r\n" + "\r\n".join(lines)).encode("utf-8")

def main(exe: Path):
    data = bytearray(exe.read_bytes())
    start = data.find(MARK)
    if start < 0:
        start = data.find(b"// Black Ops 3 Bot Mod - match side (BoiiiMVM")
    if start < 0 or data.find(MARK, start + 1) >= 0:
        sys.exit("built-in botmod.gsc not found exactly once")
    end = data.index(b"\x00", start)
    room = end - start
    text = compact((ROOT / "botmod/botmod.gsc").read_text(encoding="utf-8"))
    if len(text) > room:
        sys.exit(f"script is {len(text)} bytes, only {room} fit")
    data[start:end] = text + b" " * (room - len(text))
    exe.write_bytes(bytes(data))
    print(f"embedded {len(text)} of {room} bytes")

if __name__ == "__main__":
    main(Path(sys.argv[1]))
