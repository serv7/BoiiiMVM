# Bot mod tutorial

The bot mod builds on the BOIII bot mod created by [@luslex on X](https://x.com/luslex). Host a **private custom multiplayer game**. Press **Insert** to open its menu and **middle mouse** to spawn a bot at the point under your crosshair. The original menu's Keybinds page lets you change its own shortcuts; these are separate from the theater binds under Tab.

| Default | Action |
| --- | --- |
| Insert | Open or close the bot menu |
| Middle mouse | Spawn a bot where you aim |
| H | Save your current position |
| J | Load the saved position / respawn there |
| K | Kill your player to reset a take |

The **Bots** page can spawn or remove bots, switch them between friendly and enemy teams, make them look toward your position once or stare continuously, and save or load your position. Use **Loadout** to choose the weapon and camo held by bots or give a selected weapon and camo to yourself. Use **Specialist** to pick specialist, body, and head skins for bots or yourself. The small color circle in the menu changes its accent color. A **Keybinds** page remaps bot-menu actions.

The original console commands include `botmod_menu`, `botmod_look`, `botmod_stare 1`/`0`, `botmod_remove`, `botmod_refresh`, and `botmod_team 1`/`0`. The bot mod stores its settings in `boiii_players\botmod.json`. These commands and the on-screen menu are intended for private custom matches. The BOIII theater plugin is a separate tool for reviewing the saved film and building cinematics.
