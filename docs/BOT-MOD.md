# Bot mod tutorial

The bot mod builds on the BOIII bot mod created by [@luslex on X](https://x.com/luslex). Host a **private custom multiplayer game**. Press **Insert** to open its menu and **middle mouse** to spawn a bot at the point under your crosshair. The original menu's Keybinds page lets you change its own shortcuts; these are separate from the theater binds under Tab.

| Default | Action |
| --- | --- |
| Insert | Open or close the bot menu |
| Middle mouse | Spawn a bot where you aim |
| H | Save your current position |
| J | Load the saved position / respawn there |
| K | Kill your player to reset a take |

The **Bots** page can spawn or remove bots, switch them between friendly and enemy teams, make them look toward your position once or stare continuously, and save or load your position. Use **Loadout** to choose the weapon and camo held by bots or give a selected weapon and camo to yourself. Use **Specialist** to pick the specialist and outfit for bots or yourself (see below). The small color circle in the menu changes its accent color. A **Keybinds** page remaps bot-menu actions.

The original console commands include `botmod_menu`, `botmod_look`, `botmod_stare 1`/`0`, `botmod_remove`, `botmod_refresh`, and `botmod_team 1`/`0`. The bot mod stores its settings in `boiii_players\botmod.json`. These commands and the on-screen menu are intended for private custom matches. The BOIII theater plugin is a separate tool for reviewing the saved film and building cinematics.

## Specialist page

The Specialist page lists every outfit by its in-game name and shows a picture of the one under the mouse; the picture stays on the selected outfit when nothing is hovered. Picking an outfit sets the matching body and head in one click.

Tick **Separate Head & Body Selection** to mix parts: **Body** and **Head** above the list choose which part it changes, and the picture shows the chosen head above the chosen body with both names. Turning the option off again keeps a mixed pair until you pick a complete outfit. Blackjack has no named outfits, so his two entries are numbered.

The names were read from the game's own outfit table, where body N and helmet N of a specialist carry the same name, and the pictures were rendered on the game's Personalization screen with exactly those two parts (344 outfits). They live in `MVM/Theater/outfit-previews` (`catalog.txt`, `h<specialist>_<outfit>.jpg`, `..._head.jpg`). A missing picture or name falls back to "No picture" or a numbered entry. The page needs the supplied `boiii.exe`; with any other client build the original page with numbered Body and Head boxes is shown. `tools/outfits` holds the scripts that produced the set.

## Unfreeze bots (0.9.0-test14)

**Unfreeze bots** (bottom of the Specialist page, or the console command `botmod_unfreeze`) lets every living bot move and fight. It lasts one life: a killed bot respawns on its spawn spot, frozen again, and newly spawned bots are always frozen. Bots now run at the hardest difficulty (`bot_difficulty 3`, veteran).

boiii.exe carries its own copy of `botmod.gsc` and rewrites `boiii/custom_scripts/mp/botmod.gsc` from it on every launch, so editing the installed file does nothing. After changing `botmod/botmod.gsc`, run `tools/embed_botmod.py <game>\boiii.exe` (comments are stripped so it fits the fixed-size resource).
