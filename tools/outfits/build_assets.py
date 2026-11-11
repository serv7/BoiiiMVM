"""Turn the captured renders and the name dump into assets/outfit-previews.

How the set was made (11 November 2026, BOIII client, game on the Personalization screen):
1. lua_exec dump_names.lua in the game console, then readdump.py -> outfits_raw.txt (kept here).
2. shoot.py: steps the Personalization character through every body + helmet pair with the same
   index and saves a 1200x1330 crop of each (h<specialist>_b<body>_m<helmet>.png).
3. This script: 360x399 picture and 176x176 head close-up per outfit, plus catalog.txt.
An outfit only gets a name when the game lists the body and the helmet under the same name key.
usage: build_assets.py <folder with the captured PNGs>"""
import collections, os, sys
from PIL import Image

here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, "..", "..", "assets", "outfit-previews")
masters = sys.argv[1]
os.makedirs(out, exist_ok=True)
rows = [l.rstrip("\n").split("|") for l in open(os.path.join(here, "outfits_raw.txt"), encoding="utf-8") if l.strip()]
clean = lambda s: s.replace("\x15", "").replace("\x14", "").strip()  # the game wraps localized text in these
H = {int(r[1]): clean(r[4]) for r in rows if r[0] == "H"}
B = collections.defaultdict(dict); M = collections.defaultdict(dict)
for r in rows:
    if r[0] == "B": B[int(r[1])][int(r[2])] = (r[3], clean(r[4]))
    if r[0] == "M": M[int(r[1])][int(r[2])] = (r[3], clean(r[4]))
lines = ["# BoiiiMVM outfit catalog. Read from the game (Engine.GetHeroCustomizationTable), English names.",
         "# S|specialist index|name     O|specialist index|outfit index|name (empty = no official name)",
         "# Outfit N is body style N with helmet style N; the game lists both under the same name."]
for h in sorted(H):
    lines.append(f"S|{h}|{H[h]}")
    for i in range(max(len(B[h]), len(M[h]))):
        b = B[h].get(i); m = M[h].get(i)
        name = b[1] if (b and m and b[0] == m[0]) else ""
        if name.upper() in ("BODY", "HEAD"): name = ""
        lines.append(f"O|{h}|{i}|{name}")
        im = Image.open(os.path.join(masters, "h%d_b%02d_m%02d.png" % (h, min(i, len(B[h]) - 1), min(i, len(M[h]) - 1))))
        assert im.size == (1200, 1330)
        im.resize((360, 399), Image.LANCZOS).save(os.path.join(out, "h%d_%02d.jpg" % (h, i)), quality=86, optimize=True)
        im.crop((432, 0, 912, 480)).resize((176, 176), Image.LANCZOS).save(os.path.join(out, "h%d_%02d_head.jpg" % (h, i)), quality=86, optimize=True)
open(os.path.join(out, "catalog.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
print(len(lines) - 3 - len(H), "outfits")
