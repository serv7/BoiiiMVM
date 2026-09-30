# Installation and first run

## Requirements

You need Windows 10/11 x64 and your own installed copy of Call of Duty: Black Ops III. This download does not include the game. It includes a customized BOIII client, the bot scripts, the theater plugin, BOIII runtime data, and FFmpeg for video recording. ReShade is optional and is not bundled.

1. Download `BO3-Theater-and-Bot-Toolkit-v0.8.1.zip` from the repository's Releases page. Extract it to a temporary folder. You should see exactly `boiii`, `MVM`, `boiii.exe`, and `README.md`.
2. Close the game and BOIII. Back up the `boiii.exe` and `boiii` folder already in the same folder as your `BlackOps3.exe`. Also back up `%LOCALAPPDATA%\boiii\data` if present. Preserve your personal configs, recordings, and film files.
3. Copy the extracted `boiii.exe`, `boiii` folder, and `MVM` folder into the folder containing `BlackOps3.exe`. Merge folders and replace the supplied files. Do not copy the enclosing ZIP folder.
4. Open `%LOCALAPPDATA%\boiii` using Win+R. Copy `boiii\data` from the extracted ZIP into that location so the result is `%LOCALAPPDATA%\boiii\data`. Merge with the backup as needed. BOIII reads these runtime UI assets from AppData.
5. Launch the supplied `boiii.exe` from the BO3 game folder with `-noupdate` to keep this compatible build in place. Use its private custom match for the bot mod or load a saved multiplayer film for the theater tools.

The bot menu opens with **Insert** in a private custom match. The theater HUD appears at the bottom of a film and **Tab** opens the scene editor. See [Bot mod](BOT-MOD.md) and [Theater](THEATER.md).

## Checks and troubleshooting

If the old controller-style theater UI appears, confirm you launched the supplied `boiii.exe`, not BO3 directly. Check `boiii\plugins\bo3_theater.dll` in the game folder. Remove any duplicate theater DLL from `%LOCALAPPDATA%\boiii\plugins`; avoid launching with `-noplugins`. The logs are `boiii_players\plugins.log` and `MVM\theater.log` beneath the game folder. A build mismatch leaves the original interface active and explains why in the log. The tested `BlackOps3.exe` SHA-256 for this build is `66B95EB4667BD5B3B3D230E7BED1D29CCD261D48CA2699F01216C863BE24FF44`.

If recording fails, confirm `MVM\Theater\ffmpeg.exe` and all its adjacent DLLs were extracted. Look in `MVM\Theater\recordings\ffmpeg-last.log`. If ReShade effects do not appear, confirm you installed a compatible add-on-enabled ReShade and that its effects are active in-game. The capture falls back to the game image when the post-effects callback is unavailable.

If the bot menu does not appear, confirm `boiii\custom_scripts\mp\botmod.gsc` and `botmod_ai.gsc` are installed, that you launched the bundled client, and that you are hosting a private custom game.

To restore your prior setup, close BOIII, remove `boiii\plugins\bo3_theater.dll`, and restore the client, scripts, and runtime data from your backups. Removing only the theater DLL restores BO3's native theater UI while leaving the other customized client files in place.
