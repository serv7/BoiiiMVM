"""Create a small, UI-only update ZIP for an existing BoiiiMVM installation."""
from pathlib import Path
import hashlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "assets/client-ui/mvm_client_preferences/__init__.lua"
output = ROOT / "dist/BoiiiMVM-client-ui-update-2026-11-02.zip"
readme = """# BoiiiMVM client UI update — 2 November 2026

For an existing BoiiiMVM installation:

1. Close BOIII.
2. Copy this ZIP's boiii folder into the BO3 game folder containing boiii.exe.
   Merge folders. The addon belongs in boiii/ui_scripts/mvm_client_preferences.
3. Launch your existing boiii.exe. Press Cancel if the BOIII Patch Installer appears.

Escape → Change Team is restored in public multiplayer when the server permits
team changes. Existing killcam restrictions still apply.

safeArea_horizontal and safeArea_vertical are set to 0.88 on frontend/in-game
UI startup, so restarting the game reapplies your preferred safe area.

To remove this addon, remove only boiii/ui_scripts/mvm_client_preferences and
restart BOIII. This ZIP contains no executable, theater DLL, or bot scripts.

Verified with offline Lua 5.1 checks, including the existing BOIII party scripts.
In-game testing remains with the owner.
"""
entries = {
    "boiii/ui_scripts/mvm_client_preferences/__init__.lua": source.read_bytes(),
    "README.md": readme.encode("utf-8"),
}
output.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
    for name, payload in entries.items():
        archive.writestr(name, payload)
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    assert set(archive.namelist()) == set(entries)
    for name, payload in entries.items():
        assert archive.read(name) == payload
print(output)
print(f"Verified {output.stat().st_size} bytes; addon SHA256 {hashlib.sha256(source.read_bytes()).hexdigest()}")
