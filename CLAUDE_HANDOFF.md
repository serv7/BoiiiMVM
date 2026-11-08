# BoiiiMVM source handoff (Claude)

This ZIP is a source snapshot of the current BoiiiMVM working tree, prepared 2026-11-08. It contains the BO3 theater plugin source, BOIII client UI addon source, bot GSC scripts, native build/test projects, required ImGui/kiero/MinHook helper sources, documentation, GIF previews, fog presets, and packaging scripts. Git history and generated build/runtime artifacts are not included.

## Build the theater plugin

1. On Windows, install Visual Studio C++ Build Tools with the x64 C++ workload, Windows 10 SDK, and the `v145` MSVC platform toolset.
2. In PowerShell at this folder, run `./build.ps1`.
3. The plugin is written to `build/bo3_theater.dll`.
4. Run native checks with `./tools/test.ps1`. These compile and run offline logic/native/UI checks; they do not launch BO3 or validate live engine behavior.

`theater.vcxproj` compiles bundled dependencies under `reMVM-t7/reMVM/helpers/{ImGui,kiero,MH}`. `src/vendor` contains the project's JSON and ReShade API headers and license files.

## Other project pieces

- `botmod/`: BO3 GSC bot-mod scripts. The original @luslex bot-menu executable was supplied in binary form; its matching menu source was not supplied.
- `assets/client-ui/`: Lua addon; install with `./install.ps1 -GameDirectory "C:\path\to\Call of Duty Black Ops III" -ClientUiOnly`.
- `assets/death-previews/`: GIF reference previews used by the animation UI.
- `assets/fog-presets/`: scene-editor presets.
- `docs/`: usage guides, install tutorial, project images, and recent test notes.
- `tests/client_ui.py`: optional Lua checks require Python package `lupa`; `--party-directory` can point to the game's `boiii/data/ui_scripts/party` folder for extra compatibility checks.

## Important binary/build context

This is source only. It intentionally excludes BOIII/Black Ops 3 executables, plugin DLLs, FFmpeg executables or DLLs, BOIII runtime data, game assets, recordings, crash dumps, and the local `MVM/` runtime package. Use the matching BOIII client/game build and runtime assets described in `docs/BUILD.md` and `README.md`; don't substitute a different game executable and assume the hard-coded engine profiles remain valid.

For reference, the previously tested installed `boiii.exe` SHA-256 was `7BBCA68E116F982D7918AAD671D6678AB5520DC5CEEE8AB359ED62D7810D40CF`. The separately supplied download `boiii.exe` was `37B696E5C0FBC10715A2B827AB67B294BF7C4F5254A47AF7D296D8D5E81610B8` and is a different binary. Neither executable is included. The latest test build in the project history was test9; some in-game behavior still requires user verification. See `docs/0.9.0-TESTING.md` for feature notes and remaining checks.

This is a source snapshot, not a complete game installation or a reproducible runtime release. `docs/BUILD.md` explains the runtime packaging inputs. Root `LICENSE` and `docs/CREDITS.md` identify project licensing/credits; third-party sources retain upstream notices where present.