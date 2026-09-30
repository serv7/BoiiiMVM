// Black Ops 3 Bot Mod - match side.
//
// boiii.exe installs this file into boiii/custom_scripts/mp/ on launch, so
// edit the copy in the repo (src/client/resources/botmod), not the installed
// one. It runs on the host, so bots need a private match.
//
// The in-game menu keeps its settings in botmod_* dvars. Actions arrive as
// console commands (menu buttons, keybinds or typed), queued by addcommand.
// Built on the earlier mvm.gsc bot mod.

function autoexec botmod_init()
{
	level.botmod_bots = [];

	// The stock private-match bot fill (monitor_bot_team_population in
	// _bot.gsc) would add bots of its own
	setdvar("bot_maxallies", 0);
	setdvar("bot_maxaxis", 0);
	setdvar("bot_maxfree", 0);

	// Weapons don't drop on death, so there is never a gun on the floor for a
	// bot to pick up into the wrong slot
	level.disableweapondrop = 1;

	addcommand("botmod_spawn", ::cmd_spawn);
	addcommand("botmod_look", ::cmd_look);
	addcommand("botmod_save", ::cmd_save);
	addcommand("botmod_load", ::cmd_load);
	addcommand("botmod_suicide", ::cmd_suicide);
	addcommand("botmod_remove", ::cmd_remove);
	addcommand("botmod_refresh", ::cmd_refresh);
	addcommand("botmod_applyme", ::cmd_apply_me);
	addcommand("botmod_giveme", ::cmd_give_me);
	addcommand("botmod_camome", ::cmd_camo_me);

	level thread watch_players();
	level thread write_character_catalog();

	trace("script started");
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

// Spawns a bot where the host is aiming, facing the host, using the menu's
// current weapon, camo and specialist
function cmd_spawn(args)
{
	trace("cmd_spawn");
	host = get_host();
	if (!isdefined(host) || !isalive(host))
		return;

	position = aim_spot(host);
	if (!isdefined(position))
	{
		trace("no aim spot, using the host's position");
		position = host.origin;
	}

	bot = addtestclient();
	if (!isdefined(bot))
	{
		trace("addtestclient failed");
		host iprintln("^1Bot Mod: no free player slot for another bot");
		return;
	}
	trace("bot added");

	// _bot_loadout.gsc's on_bot_connect() builds a random create-a-class for
	// every custom slot, and an incomplete slot in this build hands undefined
	// to botclassadditem ("pair 'customN' and undefined"). It skips all of
	// that when this is already set. The loadout is ours anyway.
	bot.pers["bot_loadout"] = 1;
	bot botsetrandomcharactercustomization();

	if (level.teambased)
	{
		team = host.team;
		if (getdvarint("botmod_team", 0) == 0)
			team = other_team(host.team);
		bot.pers["team"] = team;
	}

	bot.botmod_bot = true;
	bot.botmod_origin = position;
	// Standing eye height; built from components (see aim_spot)
	bot.botmod_angles = angles_to_point((position[0], position[1], position[2] + 60), host geteye());
	bot read_loadout_settings();

	level.botmod_bots[level.botmod_bots.size] = bot;
	bot thread bot_life();
}

// Where the host's crosshair meets the world, stepped back off walls so the
// bot isn't inside one, then dropped onto the floor below.
//
// No constant vectors like (0, 0, 8) in here: boiii's compiler embeds those
// in the script (GetVector), and after the first spawn that corrupted the
// vector math around them, so every later bot got an undefined spot. Vectors
// built from their components at runtime are fine.
function aim_spot(host)
{
	eye = host geteye();
	forward = anglestoforward(host getplayerangles());
	hit = bullettrace(eye, eye + forward * 8000, 0, host);

	start = hit["position"] - forward * 16;
	top = (start[0], start[1], start[2] + 8);
	bottom = (start[0], start[1], start[2] - 4000);

	ground = bullettrace(top, bottom, 0, host);
	if (!isdefined(ground["position"]))
		trace("aim: no floor below the crosshair");

	return ground["position"];
}

// One-shot: every bot turns to face the host and holds that
function cmd_look(args)
{
	trace("cmd_look");
	host = get_host();
	if (!isdefined(host))
		return;

	target = host geteye();
	foreach (bot in get_bots())
		bot.botmod_angles = bot angles_to_point(bot geteye(), target);
}

function cmd_save(args)
{
	trace("cmd_save");
	host = get_host();
	if (!isdefined(host) || !isalive(host))
		return;

	save = spawnstruct();
	save.origin = host.origin;
	save.angles = host getplayerangles();
	save.stance = host getstance();
	level.botmod_save = save;

	host iprintln("^3Position saved");
}

function cmd_load(args)
{
	trace("cmd_load");
	host = get_host();
	if (!isdefined(host) || !isalive(host))
		return;

	if (!isdefined(level.botmod_save))
	{
		host iprintln("^1No saved position yet");
		return;
	}

	host teleport_to_save();
}

function cmd_suicide(args)
{
	trace("cmd_suicide");
	host = get_host();
	if (isdefined(host) && isalive(host))
		host suicide();
}

function cmd_remove(args)
{
	trace("cmd_remove");
	foreach (player in getplayers())
	{
		if (player istestclient())
			player botdropclient();
	}

	level.botmod_bots = [];
}

// Gives every existing bot the menu's current weapon, camo and specialist
function cmd_refresh(args)
{
	trace("cmd_refresh");
	foreach (bot in get_bots())
	{
		bot read_loadout_settings();
		if (isalive(bot))
		{
			bot apply_character();
			bot apply_weapon();
		}
	}
}

// Gives the host the menu's current specialist and skins, now and after
// every respawn
function cmd_apply_me(args)
{
	trace("cmd_apply_me");
	host = get_host();
	if (!isdefined(host))
		return;

	host.botmod_character = getdvarint("botmod_specialist", -1);
	host.botmod_body = getdvarint("botmod_body", 0);
	host.botmod_head = getdvarint("botmod_head", 0);

	if (isalive(host))
		host apply_character();
}

// Swaps the weapon in the host's hands for the menu's weapon and camo
function cmd_give_me(args)
{
	trace("cmd_give_me");
	host = get_host();
	if (!isdefined(host) || !isalive(host))
		return;

	weapon = getweapon(getdvarstring("botmod_weapon", "ar_standard"));
	if (!isdefined(weapon) || weapon == level.weaponnone)
	{
		host iprintln("^1Bot Mod: that weapon isn't available on this map");
		return;
	}

	current = host getcurrentweapon();
	if (isdefined(current) && current != level.weaponnone)
		host takeweapon(current);

	// Already carried in the other slot: replace that copy too, so the player
	// doesn't end up with two
	if (host hasweapon(weapon))
		host takeweapon(weapon);

	// No camo; botmod_camome puts one on afterwards
	host giveweapon(weapon, host calcweaponoptions(0, 0, 0, 0, 0));
	host givemaxammo(weapon);
	host switchtoweaponimmediate(weapon);
}

// Puts the menu's camo on the weapon in the host's hands, keeping the weapon,
// its attachments and its ammo
function cmd_camo_me(args)
{
	trace("cmd_camo_me");
	host = get_host();
	if (!isdefined(host) || !isalive(host))
		return;

	weapon = host getcurrentweapon();
	if (!isdefined(weapon) || weapon == level.weaponnone)
		return;

	host updateweaponoptions(weapon, host calcweaponoptions(getdvarint("botmod_camo", 0), 0, 0, 0, 0));
}

// ---------------------------------------------------------------------------
// Bots
// ---------------------------------------------------------------------------

function read_loadout_settings()
{
	self.botmod_weapon = getdvarstring("botmod_weapon", "ar_standard");
	self.botmod_camo = getdvarint("botmod_camo", 0);
	self.botmod_character = getdvarint("botmod_specialist", -1);
	self.botmod_body = getdvarint("botmod_body", 0);
	self.botmod_head = getdvarint("botmod_head", 0);
}

// Every life: put the bot back on its spot, frozen, with its loadout
function bot_life()
{
	self endon("disconnect");

	for (;;)
	{
		self waittill("spawned_player");
		waittillframeend;
		trace("bot spawned");

		// Never leave these undefined: the per-frame pose code would error
		// every frame, and that flood of errors makes the game stutter
		if (!isdefined(self.botmod_origin))
			self.botmod_origin = self.origin;
		if (!isdefined(self.botmod_angles))
			self.botmod_angles = self getplayerangles();

		self setorigin(self.botmod_origin);
		self setplayerangles(self.botmod_angles);
		self freezecontrols(1);

		self thread hold_pose();
		self thread bot_loadout();
	}
}

// Kept in its own thread: if setting the specialist or weapon fails, only
// this thread stops and the bot still holds its spot
function bot_loadout()
{
	self endon("disconnect");
	self endon("death");
	self endon("spawned_player");

	self apply_character();
	trace("bot specialist set");
	self apply_weapon();
	trace("bot weapon set");

	self thread weapon_watchdog();
}

// Frozen bots can't move or shoot, but the bot AI still turns their view
// (like frozen players at match start). Re-applying the pose every frame
// keeps them where the menu put them, and puts them back if anything moved
// them. With botmod_stare on they follow the host instead.
function hold_pose()
{
	self endon("disconnect");
	self endon("death");
	self endon("spawned_player");

	moved_logged = false;
	for (;;)
	{
		if (getdvarint("botmod_stare", 0))
		{
			host = get_host();
			if (isdefined(host) && isalive(host))
				self.botmod_angles = self angles_to_point(self geteye(), host geteye());
		}

		if (distancesquared(self.origin, self.botmod_origin) > 16 * 16)
		{
			if (!moved_logged)
			{
				trace("bot was moved " + int(distance(self.origin, self.botmod_origin)) + " units off its spot, putting it back");
				moved_logged = true;
			}
			self setorigin(self.botmod_origin);
		}

		self setplayerangles(self.botmod_angles);

		wait 0.05;
	}
}

function apply_weapon()
{
	weapon = getweapon(self.botmod_weapon);
	if (!isdefined(weapon) || weapon == level.weaponnone)
		weapon = getweapon("ar_standard");

	self takeallweapons();
	self clearperks();
	self giveweapon(weapon, self calcweaponoptions(self.botmod_camo, 0, 0, 0, 0));

	// On a respawn the stock loadout calls setspawnweapon(<class gun>, true),
	// a forced switch the game carries out after this script has run, so the
	// class gun ended up in hand and ours on the bot's back. Queue our own
	// forced switch the same way; it's the later call, so it wins. (A plain
	// weapon switch doesn't work on a frozen bot.)
	self setspawnweapon(weapon, true);
	self switchtoweaponimmediate(weapon);
	self.botmod_weapon_object = weapon;
}

// The spawn loadout code can hand out a different weapon a moment after
// spawning; put ours back if that happens
function weapon_watchdog()
{
	self endon("disconnect");
	self endon("death");
	self endon("spawned_player");

	for (;;)
	{
		wait 0.25;

		if (self getcurrentweapon() != self.botmod_weapon_object)
			self apply_weapon();
	}
}

// botmod_specialist -1 keeps the random look the bot connected with
function apply_character()
{
	if (!isdefined(self.botmod_character) || self.botmod_character < 0)
		return;

	mode = currentsessionmode();
	body = clamp_index(self.botmod_body, getcharacterbodymodelcount(self.botmod_character, mode));
	head = clamp_index(self.botmod_head, getcharacterhelmetmodelcount(self.botmod_character, mode));

	self setcharacterbodytype(self.botmod_character);
	self setcharacterbodystyle(body);
	self setcharacterhelmetstyle(head);
}

function get_bots()
{
	bots = [];
	foreach (bot in level.botmod_bots)
	{
		if (isdefined(bot))
			bots[bots.size] = bot;
	}

	level.botmod_bots = bots;
	return bots;
}

// ---------------------------------------------------------------------------
// Host
// ---------------------------------------------------------------------------

function watch_players()
{
	for (;;)
	{
		level waittill("connected", player);

		if (!player istestclient())
			player thread host_life();
	}
}

// Save & load: after dying, respawn on the saved spot while botmod_saveload
// is on. Also keeps a specialist picked with "Apply to me".
function host_life()
{
	self endon("disconnect");

	for (;;)
	{
		self waittill("spawned_player");
		waittillframeend;
		trace("host spawned");

		if (isdefined(self.botmod_character))
			self apply_character();

		if (getdvarint("botmod_saveload", 1) && isdefined(level.botmod_save))
			self teleport_to_save();
	}
}

function teleport_to_save()
{
	self setorigin(level.botmod_save.origin);
	self setplayerangles(level.botmod_save.angles);
	self setstance(level.botmod_save.stance);
}

// ---------------------------------------------------------------------------
// Menu data
// ---------------------------------------------------------------------------

// Lists every specialist with its number of body and head skins for the menu
// (boiii/scriptdata/botmod_characters.txt), one per line:
// <body type>|<display name reference>|<body count>|<head count>
function write_character_catalog()
{
	wait 1;

	mode = currentsessionmode();
	text = "";

	foreach (bodytype in getallcharacterbodies(mode))
	{
		text += bodytype + "|" + getcharacterdisplayname(bodytype, mode) + "|";
		text += getcharacterbodymodelcount(bodytype, mode) + "|";
		text += getcharacterhelmetmodelcount(bodytype, mode) + "\n";
	}

	writefile("botmod_characters.txt", text);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Temporary while the mod is tested: boiii/scriptdata/botmod_trace.txt
function trace(message)
{
	appendfile("botmod_trace.txt", gettime() + " " + message + "\n");
}

function get_host()
{
	foreach (player in getplayers())
	{
		if (player ishost())
			return player;
	}

	players = getplayers();
	if (players.size > 0)
		return players[0];

	return undefined;
}

function other_team(team)
{
	if (team == "allies")
		return "axis";

	return "allies";
}

function angles_to_point(from, to)
{
	return vectortoangles(to - from);
}

function clamp_index(index, count)
{
	if (count <= 0 || index < 0)
		return 0;
	if (index >= count)
		return count - 1;

	return index;
}
