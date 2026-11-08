# Building and source scope

GitHub's “Source code” download contains the theater plugin C++ code, `botmod.gsc` and `botmod_ai.gsc`, build scripts, tests, and the ImGui/MinHook/kiero helper files required by `theater.vcxproj`. The original @luslex bot-menu executable was received in binary form; its matching menu source was not supplied. The linked [upstream BOIII repository](https://github.com/Ezz-lol/boiii-free) is not a drop-in source match for this customized `boiii.exe`.

To build the theater DLL, install Visual Studio C++ tools for x64 with the Windows SDK and the `v145` platform toolset used by `theater.vcxproj`. Run `build.ps1` from the repository root. The result is `build\bo3_theater.dll`. To install it into your own BO3 directory, use `install.ps1 -GameDirectory "C:\path\to\Call of Duty Black Ops III"`. The runtime release already has a tested DLL; users do not need a compiler.

`tools/package_public_release.py` constructs the four-item runtime ZIP from an installed compatible client, the built DLL, local BOIII runtime data, and the bundled FFmpeg files. The script verifies the installed and built plugin hashes match, tests ZIP integrity, and rejects game executables and generated logs. The repository and GitHub's generated source downloads are separate from that runtime ZIP.

Run `tools/test.ps1` from PowerShell to build and run the camera logic, saved-preference, native-call/CFG, scene-restoration, cursor-hook, and offscreen UI checks. Preview images are written under `build/previews`. These checks do not launch BO3 or verify live engine behavior. New test-build features and remaining game checks are documented in [0.9.0-TESTING.md](0.9.0-TESTING.md).

The client UI addon in `assets/client-ui` is installed under the game's `boiii/ui_scripts` directory. It needs no DLL rebuild. Use `install.ps1 -GameDirectory "C:\path\to\Call of Duty Black Ops III" -ClientUiOnly` to install just this addon. `tests/client_ui.py` uses Lupa's Lua 5.1 runtime for offline checks; pass `--party-directory "C:\path\to\boiii\data\ui_scripts\party"` to exercise the actual party-menu generator and constructor wrappers too. These checks do not launch BO3.
