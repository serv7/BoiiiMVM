# Client UI update — 2 November 2026

In a multiplayer match, open **Escape → Change Team**. The addon restores the public/ranked team-change UI check when the pause menu is constructed or its options refresh. It uses the existing Choose Team action and respects the server's `allowInGameTeamChange` setting and the existing killcam restrictions.

`safeArea_horizontal` and `safeArea_vertical` are set to **0.88** whenever BOIII initializes its frontend or in-game UI. These engine dvars are registered without archive flags, so the addon reapplies the preference after initialization instead of relying on config saving. It uses no timer or background loop. Opening a pause menu does not repeatedly reset the values.

**END GAME (8 November 2026).** As the host of a live multiplayer match, open **Escape → END GAME** to finish the match normally. This is the way to end a custom game so its film is saved. Leaving with **Main Menu** or `/disconnect` does not finish the match. Guests, films, zombies and campaign keep BOIII's own pause menu.

The source is `assets/client-ui/mvm_client_preferences/__init__.lua`. Install it at `boiii/ui_scripts/mvm_client_preferences/__init__.lua` under the BO3 game folder. This is BOIII's game-local UI addon directory; keep it separate from the runtime-data copy under `boiii/data`. Launch the existing **boiii.exe** as usual. The reference executable supplied for comparison is not required by this addon.

From a source checkout, `install.ps1 -GameDirectory "C:\path\to\Call of Duty Black Ops III" -ClientUiOnly` installs only the UI addon and backs up an existing copy. The regular installer and runtime packager also include it. To remove these preferences later, remove only the `boiii/ui_scripts/mvm_client_preferences` folder and restart BOIII.

Offline Lua 5.1 checks cover public-match menu reconstruction, datasource refresh, server denial, killcam restrictions, non-MP/demo/frontend behavior, repeated loads, and startup safe-area resets, including integration with the installed party scripts. No game or public server was opened for testing.
