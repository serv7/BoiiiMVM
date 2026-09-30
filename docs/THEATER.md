# Theater and cinematic tutorial

BO3's native film editor exposes a controller-oriented strip of controls. This overlay gives keyboard shortcuts for the common actions and a mouse-driven Tab panel. Load a saved multiplayer film through the bundled BOIII client. The bottom strip shows current and total demo time, marker positions, playback speed, camera mode, FOV, and roll.

## Playback and camera path

| Default | Action |
| --- | --- |
| F2 / F4 | Toggle game HUD / theater controls independently |
| F3 | Cycle first person, third person, free camera |
| Space | Pause or play the film |
| Left / right arrows | Seek backward or forward |
| Up / down arrows | Lower or raise playback timescale, from 0.01× to 10× |
| Left / right mouse in free camera | Dolly Camera / Edit Camera mode |
| F | Place a camera, edit a marker you aim at, or finish repositioning |
| L | Delete all camera markers |
| Ctrl+M | Seek to the first camera and enter dolly mode |
| Mouse wheel / Ctrl+wheel | Camera roll / FOV |
| Q / E in free camera | Move vertically down / up |
| Tab | Open the scene editor and release the cursor |
| F5 | Start or stop video recording |
| Escape / F10 | Open exit-film confirmation (or close an open panel) |

Enter **Free Camera** with F3. Pause at the first desired film time, aim the free camera, and press F. Move to another time and position and press F again. At least two markers at distinct film times are needed for dolly playback. You can continue adding markers. When aiming directly at an existing marker, F opens its edit popup: reposition, delete, or apply the current lens values. Reposition, then press F to commit the new pose. Markers are shown on the timeline and in the world. Left mouse switches to dolly playback; right mouse returns to editing. Position follows a smoothed curve through the markers, while BO3's native dolly controls orientation to avoid unintended spins. Save or import a path through the scene editor when needed.

The **Sliders** tab adjusts free-camera movement speed down to 0.0001×, including Q/E vertical motion, and field of view. Camera speed is independent of playback timescale. It also has **Enable useful dvars**, a reversible toggle for `r_DedicatedPlayerShadowCull 0`, `r_DedicatedPlayerSunShadowResolution 0`, and `demo_dollycamHighlightThresholdDistance 3`. Switching it off restores the values seen when it was enabled. Use **Exit film to main menu** or the Escape/F10 confirmation; `/disconnect` can reload the film in this client.

## Scene editor

Tab opens a narrow panel on the left so the scene stays visible. **No background** removes most panel shading. Sections are Sliders, Fog, Sun, Lights, Misc, Configs, Settings, and Binds. Settings take effect live; drag sliders, or Ctrl-click a numeric value for precise input. Color swatches open an RGB/HDR wheel.

- **Fog:** Enable and shape base/half distance, height, haze, density, brightness, opacity, PBR amount, and lit-fog density. Pick fog and sun-haze colors, synchronize colors, load/save `.zfog` presets, or generate random fog.
- **Sun:** Set sun color, brightness, bloom, shadow aliasing, and X/Y direction. Reset to the film defaults if needed.
- **Lights:** Add or select a light, choose omnidirectional/spotlight, adjust its color/intensity/range, go to a light, and save/load `.bo3light` lighting. The native engine limit is 25 lights.
- **Misc:** Adjust exposure, sky rotation/transition, gun offsets, HUD and weapon visibility, 2D draw state, blood overlay, and timescale.
- **Configs:** Save/load a scene `.cfg` or restore captured film defaults. CFGs cover scene controls, not the camera path or recordings.
- **Settings:** Choose recording FPS (1–240), codec, and fixed-frame playback. Check recording status and output path.
- **Binds:** Reassign theater controls, including Tab and F5. These bindings are saved to `MVM\Theater\binds.json`.

Config files live in `MVM\Theater\configs`, fog presets in `MVM\Theater\fog-presets`, and lighting files in `MVM\Theater\lighting`. Several fog presets are included. The save/import camera-path controls are separate from the scene CFGs.

## Recording

Choose **ProRes 4444**, **ProRes 422 HQ**, or lossless **FFV1** in Settings, set a target frame rate, then press F5. Press F5 again to finish. Files are saved under `MVM\Theater\recordings`. Recording is video-only; no audio track is generated. ProRes 4444 outputs a 10-bit 4:4:4 `.mov` using FFmpeg's `prores_ks` profile 4. FFV1 uses `.mkv`.

The recorder attempts fixed-frame capture using BO3's `com_fixedtime_float`; the panel shows whether fixed-frame timing is active. If an add-on-enabled ReShade presents its finish-effects callback, frames are captured after effects and before this UI. Otherwise, the recorder captures the game image without those post-effects. ReShade itself and personal presets are not bundled. Do a short test recording before a long take, and wait for FFmpeg to finish writing after stopping.
