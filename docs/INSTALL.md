# Installation and first run

[![Watch the 40-second installation video](video/install-cover.jpg)](video/how-to-install.mp4)

**[Watch the installation video](video/how-to-install.mp4)** before copying files. The crucial launch steps are to run the supplied **`boiii.exe`, not `BlackOps3.exe`**, and press **Cancel** if the BOIII Patch Installer offers to download a different game executable.

## Requirements

You need Windows 10/11 x64 and an installed copy of Call of Duty: Black Ops III. The toolkit ZIP includes a customized BOIII client, the bot scripts, the theater plugin, BOIII runtime data, and FFmpeg for video recording. ReShade is optional and is not bundled.

1. Download `BoiiiMVM-v0.8.1.zip` from the repository's Releases page. Extract it to a temporary folder. You should see exactly `boiii`, `MVM`, `boiii.exe`, and `README.md`.
2. Close the game and BOIII. Back up the `boiii.exe` and `boiii` folder already in the same folder as your `BlackOps3.exe`. Also back up `%LOCALAPPDATA%\boiii\data` if present. Preserve your personal configs, recordings, and film files.
3. Copy the extracted `boiii.exe`, `boiii` folder, and `MVM` folder into the folder containing `BlackOps3.exe`. Merge folders and replace the supplied files. Do not copy the enclosing ZIP folder.
4. Open `%LOCALAPPDATA%\boiii` using Win+R. Copy `boiii\data` from the extracted ZIP into that location so the result is `%LOCALAPPDATA%\boiii\data`. Merge with the backup as needed. BOIII reads these runtime UI assets from AppData.
5. **Launch the supplied `boiii.exe`, not `BlackOps3.exe`,** from the BO3 game folder with `-noupdate`. If the **BOIII Patch Installer** prompt appears, **press Cancel**. Use a private custom match for the bot mod or load a saved multiplayer film for the theater tools.

![BOIII Patch Installer prompt: press Cancel](images/boiii-patch-prompt.png)

The bot menu opens with **Insert** in a private custom match. The theater HUD appears at the bottom of a film and **Tab** opens the scene editor. See [Bot mod](BOT-MOD.md) and [Theater](THEATER.md).

## Other troubleshooting

If recording fails, confirm `MVM\Theater\ffmpeg.exe` and all its adjacent DLLs were extracted. Look in `MVM\Theater\recordings\ffmpeg-last.log`. If ReShade effects do not appear, confirm you installed a compatible add-on-enabled ReShade and that its effects are active in-game. The capture falls back to the game image when the post-effects callback is unavailable.

If the bot menu does not appear, confirm `boiii\custom_scripts\mp\botmod.gsc` and `botmod_ai.gsc` are installed, that you launched the bundled client, and that you are hosting a private custom game.

To restore your prior setup, close BOIII, remove `boiii\plugins\bo3_theater.dll`, and restore the client, scripts, and runtime data from your backups. Removing only the theater DLL restores BO3's native theater UI while leaving the other customized client files in place.
