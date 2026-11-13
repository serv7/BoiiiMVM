# BOIII client changes

`boiii-client-changes.patch` holds the source changes behind the `boiii.exe` shipped with BoiiiMVM since 0.9.0-test15.

It applies to commit `4c1bbb7ff8331e95325384e76c346a53dfc7ab92` of the bot-mod branch of https://github.com/alxndermz3/boiii-free-demo-recording (a fork of Ezz-lol/boiii-free, GPL-3.0):

```
git checkout 4c1bbb7ff8331e95325384e76c346a53dfc7ab92
git apply boiii-client-changes.patch
```

Then build it like upstream: generate the solution with premake, and build `build\boiii.slnx` with `Release|x64`.

What the patch does:

- **Client film recording.**
  - Every online match is recorded automatically. The film is saved when the match ends or you leave through the Esc menu, and a new film starts with the next match.
  - A small RECORDING label shows in the bottom left while it records.
  - The film's Theater tile shows the real map, mode, length and date.
  - Files: `demo_client_record.cpp`, `bdPooledStorage.*`.
- **Bot menu additions moved into the client.** These used to be patched into a fixed `boiii.exe` by the theater plugin:
  - the Specialist page with outfit names and pictures;
  - the input block while the menu is open;
  - the Unfreeze bots button.
  - Files: `botmod.cpp`, `botmod_outfits.hpp`, `download_overlay.*`.
- **Hold-gun helper in films.** `mvm_corpse.csc` is loaded only for Theater playback. File: `script.cpp`.
- **No Patch Installer prompt** at launch unless `-patchprompt` is given. File: `main.cpp`.
- **Embedded bot script.** The client's built-in copy is updated to `botmod/botmod.gsc`.
