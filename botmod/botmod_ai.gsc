// Black Ops 3 Bot Mod - stock bot AI overrides.
//
// Installed next to botmod.gsc by boiii.exe. Kept in its own file so that if
// a game update ever breaks a detour, the main bot mod script still loads.

// Bots pick a random create-a-class on connect and after every death, and
// some custom class slots are incomplete in this build (script error "pair
// 'customN' and undefined"). A bot only spawns once it has picked a class,
// so bot mod bots pick the stock Assault class once and keep it; botmod.gsc
// then replaces the weapon. The earlier mvm.gsc set
// level.disableclassselection instead, but that also removes the host's
// scorestreaks.
detour bot<scripts\mp\bots\_bot.gsc>::choose_class()
{
	if (isdefined(self.botmod_class_chosen))
		return false;

	self.botmod_class_chosen = true;
	self notify("menuresponse", "ChooseClass_InGame", "class_assault");
	return true;
}
