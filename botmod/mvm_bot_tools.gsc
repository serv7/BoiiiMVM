// Additional private-match actions. Kept separate from the client's embedded botmod.gsc.
function autoexec mvm_bot_tools_init()
{
    addcommand("botmod_clearbodies", ::clear_bodies);
    thread weapon_drop_mode();
}

function weapon_drop_mode()
{
    for (;;)
    {
        level.disableWeaponDrop = GetDvarInt("mvm_weapon_drops", 0) == 0;
        wait 0.1;
    }
}

function clear_bodies(args)
{
    ClearPlayerCorpses();
    foreach (player in GetPlayers())
    {
        if (player IsHost())
            player IPrintLn("^3BoiiiMVM: bodies cleared");
    }
}
