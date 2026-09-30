# Building and source scope

GitHub's “Source code” download contains the theater plugin C++ code, `botmod.gsc` and `botmod_ai.gsc`, build scripts, tests, and the ImGui/MinHook/kiero helper files required by `theater.vcxproj`. The original @luslex bot-menu executable was received in binary form; its matching menu source was not supplied. The linked [upstream BOIII repository](https://github.com/Ezz-lol/boiii-free) is not a drop-in source match for this customized `boiii.exe`.

To build the theater DLL, install Visual Studio C++ tools for x64 with the Windows SDK and the `v145` platform toolset used by `theater.vcxproj`. Run `build.ps1` from the repository root. The result is `build\bo3_theater.dll`. To install it into your own BO3 directory, use `install.ps1 -GameDirectory "C:\path\to\Call of Duty Black Ops III"`. The runtime release already has a tested DLL; users do not need a compiler.

`tools/package_public_release.py` constructs the four-item runtime ZIP from an installed compatible client, the built DLL, local BOIII runtime data, and the bundled FFmpeg files. The script verifies the installed and built plugin hashes match, tests ZIP integrity, and rejects game executables and generated logs. The repository and GitHub's generated source downloads are separate from that runtime ZIP.
