"""Build the public four-item runtime ZIP from explicit, verified inputs.

Usage: python tools/package_public_release.py --game ".../Call of Duty Black Ops III" \
       --boiii-data "%LOCALAPPDATA%/boiii/data"
"""

import argparse
import hashlib
import json
from pathlib import Path
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.8.1"
NAME = f"BoiiiMVM-v{VERSION}.zip"
DATA_DIRS = ("gamesettings", "launcher", "lookup_tables", "scripts", "ui_scripts")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def add(entries: dict[str, bytes], name: str, path: Path) -> None:
    if not path.is_file():
        raise FileNotFoundError(path)
    if name in entries:
        raise ValueError(f"Duplicate archive entry: {name}")
    entries[name] = path.read_bytes()


def walk(entries: dict[str, bytes], source: Path, prefix: str) -> None:
    if not source.is_dir():
        raise FileNotFoundError(source)
    for path in sorted(source.rglob("*")):
        if path.is_file():
            if path.suffix.lower() in {".log", ".dmp", ".pdb", ".key", ".pem", ".pfx"}:
                raise ValueError(f"Refusing generated/private file: {path}")
            add(entries, prefix + "/" + path.relative_to(source).as_posix(), path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--boiii-data", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "dist" / NAME)
    args = parser.parse_args()
    game = args.game.resolve()
    data = args.boiii_data.resolve()
    built = (ROOT / "build/bo3_theater.dll").read_bytes()
    installed = (game / "boiii/plugins/bo3_theater.dll").read_bytes()
    if built != installed or f"BO3 Theater Keyboard UI {VERSION}".encode() not in built:
        raise ValueError("Installed theater plugin does not match the v0.8.1 build")
    if not (game / "BlackOps3.exe").is_file():
        raise FileNotFoundError("The game folder must contain the user's BlackOps3.exe")

    entries: dict[str, bytes] = {}
    add(entries, "boiii.exe", game / "boiii.exe")
    add(entries, "boiii/plugins/bo3_theater.dll", game / "boiii/plugins/bo3_theater.dll")
    for script in ("botmod.gsc", "botmod_ai.gsc"):
        add(entries, f"boiii/custom_scripts/mp/{script}", ROOT / "botmod" / script)
    for dirname in DATA_DIRS:
        walk(entries, data / dirname, f"boiii/data/{dirname}")
    for file in sorted((ROOT / "assets/ffmpeg").iterdir()):
        if file.is_file():
            add(entries, "MVM/Theater/" + file.name, file)
    for file in sorted((ROOT / "assets/fog-presets").glob("*.zfog")):
        add(entries, "MVM/Theater/fog-presets/" + file.name, file)

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    base = "https://github.com/serv7/BoiiiMVM"
    readme = readme.replace("(docs/", f"({base}/blob/main/docs/")
    readme = readme.replace(f"({base}/blob/main/docs/images/",
                            f"({base}/raw/refs/heads/main/docs/images/")
    readme += ("\n## Tested game build\n\nThe plugin was built for a `BlackOps3.exe` with SHA-256 "
               f"`{sha256((game / 'BlackOps3.exe').read_bytes())}`. "
               "The game executable is not included.\n")
    entries["README.md"] = readme.encode("utf-8")

    forbidden = ("BlackOps3.exe", "botmod_trace.txt", "botmod_characters.txt")
    if any(any(term.lower() in name.lower() for term in forbidden) for name in entries):
        raise ValueError("Archive contains a game executable or generated bot data")
    if {name.split("/", 1)[0] for name in entries} != {"boiii", "MVM", "boiii.exe", "README.md"}:
        raise ValueError("Unexpected top-level archive item")

    manifest = {
        "version": VERSION,
        "tested_game_sha256": sha256((game / "BlackOps3.exe").read_bytes()),
        "files": {name: {"bytes": len(payload), "sha256": sha256(payload)}
                  for name, payload in sorted(entries.items())},
    }
    entries["MVM/Theater/MANIFEST.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, payload in sorted(entries.items()):
            archive.writestr(name, payload)
    with zipfile.ZipFile(args.output) as archive:
        if archive.testzip() is not None or len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError("ZIP integrity check failed")
        for name, spec in manifest["files"].items():
            if sha256(archive.read(name)) != spec["sha256"]:
                raise ValueError(f"Archive hash mismatch: {name}")
    print(json.dumps({"zip": str(args.output), "bytes": args.output.stat().st_size,
                      "entries": len(entries), "client_sha256": sha256(entries["boiii.exe"]),
                      "plugin_sha256": sha256(built), "verified": True}, indent=2))


if __name__ == "__main__":
    main()
