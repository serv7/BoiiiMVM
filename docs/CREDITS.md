# Credits and third-party components

- [@luslex on X](https://x.com/luslex) created the original BOIII bot mod this combined build uses, including the Insert menu, middle-mouse bot spawning, and bot specialist, skin, weapon, and camo controls. The original bot-menu executable was supplied as a binary; its menu source was not supplied.
- [Ezz-lol/boiii-free](https://github.com/Ezz-lol/boiii-free) is the upstream BOIII client. Its GPL-3.0 license is represented by the repository's `LICENSE` file. The customized executable in the release differs from unmodified upstream.
- [reMVM-t7](https://github.com/markusrees/reMVM) informed the cinematic workflow and supplied the bundled ImGui, MinHook, and kiero helper tree. ImGui and kiero are MIT-licensed; MinHook uses a BSD-style license.
- [nlohmann/json](https://github.com/nlohmann/json) is used under MIT (see `src/vendor/JSON-LICENSE.txt`). The [ReShade add-on API](https://github.com/crosire/reshade) headers are under BSD-3-Clause (see `src/vendor/reshade/LICENSE.md`).
- The runtime package bundles an FFmpeg shared build for encoding. Its adjacent `MVM/Theater/LICENSE.txt` contains the LGPL notice; [FFmpeg source](https://github.com/FFmpeg/FFmpeg) and [Windows build provider](https://github.com/BtbN/FFmpeg-Builds) are linked for attribution. ReShade binaries are not bundled.
- Screenshots were supplied by the project owner and document their own BO3 gameplay and editor scenes.

The theater plugin code and the bundled bot GSC scripts are provided in this repository and GitHub's generated source downloads. No BO3 game executable or game data are part of this release.
