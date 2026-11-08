#using scripts\shared\callbacks_shared;
#using scripts\shared\system_shared;
#using scripts\shared\util_shared;
#namespace mvm_corpse;

// Client playback helper. Installed at custom_scripts/ so demo loads do not
// depend on the private-match mp/ script-directory selection.
function autoexec mvm_corpse_init()
{
    // Match stock client systems: autoexec registers work, and the engine
    // starts it during CodeCallback_FinalizeInitialization, after pre-init.
    system::register("mvm_corpse", undefined, &main, undefined);
}

function main()
{
    // Films only. In live matches this helper's shutdown callbacks and manager
    // thread broke the end-of-match demo summary upload, so recent games stayed
    // empty. Do nothing here unless a film is playing.
    if (!IsDemoPlaying())
        return;
    if (IsDefined(level.mvm_corpse_started))
        return;
    SetDvar("mvm_corpse_stage", "initializing");
    SetDvar("mvm_corpse_ready", 0);
    SetDvar("mvm_corpse_models", 0);
    level.mvm_corpse_started = true;
    level.mvm_corpse_entries = [];
    level.mvm_hidden_drops = [];
    level.mvm_drop_refresh = false;
    level.mvm_seed_players = true;
    level.mvm_seed_attempts = 0;
    level.mvm_corpse_serial = 0;
    level.mvm_corpse_refresh = true;
    callback::on_spawned(&on_player_spawned);
    level thread playback_manager();
}

function remember_weapon(player)
{
    id = player GetEntityNumber();
    if (!IsDefined(level.mvm_corpse_entries[id]))
    {
        level.mvm_corpse_entries[id] = SpawnStruct();
        level.mvm_corpse_entries[id].released = false;
        level.mvm_corpse_entries[id].release_attempted = false;
        level.mvm_corpse_entries[id].player_id = id;
    }
    weapon = player.weapon;
    if (IsDefined(weapon) && IsDefined(weapon.worldModel))
        level.mvm_corpse_entries[id].weapon = weapon;
    return id;
}

function on_player_spawned(localClientNum)
{
    if (!self IsPlayer())
        return;
    remember_weapon(self);
    self notify("mvm_death_observer_restart");
    if (!IsDefined(self.mvm_shutdown_registered))
    {
        self.mvm_shutdown_registered = true;
        self callback::on_shutdown(&on_player_shutdown);
    }
    self thread death_observer(localClientNum);
}

function death_observer(localClientNum)
{
    self endon("entityshutdown");
    self endon("mvm_death_observer_restart");
    self waittill("death");
    self capture_corpse(localClientNum);
}

function on_player_shutdown(localClientNum)
{
    if (self IsPlayer())
        self capture_corpse(localClientNum);
}

function capture_corpse(localClientNum)
{
    id = remember_weapon(self);
    corpse = self GetPlayerCorpse();
    if (!IsDefined(corpse))
        return;
    entry = level.mvm_corpse_entries[id];
    if (!IsDefined(entry.corpse) || entry.corpse != corpse)
    {
        detach_weapon(entry);
        if (IsDefined(entry.serial))
            restore_corpse_drops(entry.serial);
        cleanup_released_weapon(entry);
        level.mvm_corpse_serial++;
        entry.serial = level.mvm_corpse_serial;
        entry.corpse = corpse;
        entry.born = GetTime();
        entry.released = false;
        entry.release_attempted = false;
        entry.previous_position = undefined;
        level.mvm_drop_refresh = true;
        level.mvm_corpse_refresh = true;
    }
}

// The recorded item still exists in a demo. Hide its presentation while the
// replacement follows the hand; restore it when the playback override is off.
function hide_recorded_item(localClientNum)
{
    // Only called with ET_ITEM entities returned by GetEntArrayByType. Client
    // script models/triggers do not all support the network entity type field.
    // Reading self.type while walking GetEntArray caused terminal VM errors.
    if (!IsDefined(self.weapon))
        return;
    if (IsDefined(self.mvm_hidden_drop) || self IsHidden())
        return;
    foreach (entry in level.mvm_corpse_entries)
    {
        if (!IsDefined(entry.corpse) || !IsDefined(entry.weapon))
            continue;
        if (!IsDefined(entry.attached) && !entry.released)
            continue;
        if (self.weapon != entry.weapon)
        {
            if (!IsDefined(self.weapon.worldModel) || self.weapon.worldModel != entry.weapon.worldModel)
                continue;
        }
        // Match the nearby victim as well as the weapon, leaving unrelated
        // world pickups alone. No entity positions are changed in the demo.
        if (DistanceSquared(self.origin, entry.corpse.origin) > 16384)
            continue;
        if (IsDefined(self.owner) && self.owner IsPlayer() && self.owner GetEntityNumber() != entry.player_id)
            continue;
        self.mvm_hidden_drop = true;
        self.mvm_hidden_serial = entry.serial;
        self Hide();
        level.mvm_hidden_drops[level.mvm_hidden_drops.size] = self;
        return;
    }
}

function on_entity_spawned(localClientNum)
{
    if (IsDefined(level.mvm_original_entity_spawned))
        self [[level.mvm_original_entity_spawned]](localClientNum);
    if (self IsPlayer())
        self on_player_spawned(localClientNum);
    if (IsDemoPlaying() && GetDvarInt("mvm_holdgun", 0) != 0)
        level.mvm_drop_refresh = true;
}

function refresh_recorded_drops()
{
    level.mvm_drop_refresh = false;
    has_override = false;
    foreach (entry in level.mvm_corpse_entries)
    {
        if (IsDefined(entry.corpse) && (IsDefined(entry.attached) || entry.released))
        {
            has_override = true;
            break;
        }
    }
    if (!has_override)
        return;
    // ET_ITEM = 3 in this BO3 build. Ask the engine for network weapon items,
    // never probe .type on every client entity. Spawn events coalesce into one
    // snapshot; there is no world query until a held/released override exists.
    foreach (entity in GetEntArrayByType(0, 3))
        entity hide_recorded_item(0);
}

function restore_recorded_drops()
{
    foreach (entity in level.mvm_hidden_drops)
    {
        if (IsDefined(entity) && IsDefined(entity.mvm_hidden_drop))
        {
            entity Show();
            entity.mvm_hidden_drop = undefined;
            entity.mvm_hidden_serial = undefined;
        }
    }
    level.mvm_hidden_drops = [];
}

function restore_corpse_drops(serial)
{
    foreach (entity in level.mvm_hidden_drops)
    {
        if (IsDefined(entity) && IsDefined(entity.mvm_hidden_drop) && entity.mvm_hidden_serial == serial)
        {
            entity Show();
            entity.mvm_hidden_drop = undefined;
            entity.mvm_hidden_serial = undefined;
        }
    }
}

function cleanup_released_weapon(entry)
{
    if (IsDefined(entry.dropped) && IsDynEntValid(entry.dropped))
        SetDynEntEnabled(entry.dropped, false);
    entry.dropped = undefined;
}

// The held weapon is attached to the body's own model, like stock gib and
// character code does. A separate linked script_model collided with the
// ragdoll when it reached the floor and threw the body across the map.
function attach_weapon(entry)
{
    model = entry.weapon.worldModel;
    if (!(entry.corpse IsAttached(model, "tag_weapon_right")))
        entry.corpse Attach(model, "tag_weapon_right");
    entry.attached = model;
}

function detach_weapon(entry)
{
    if (IsDefined(entry.attached) && IsDefined(entry.corpse) && entry.corpse IsAttached(entry.attached, "tag_weapon_right"))
        entry.corpse Detach(entry.attached, "tag_weapon_right");
    entry.attached = undefined;
}

function delete_with_corpse(corpse, serial, entry)
{
    corpse waittill("entityshutdown");
    restore_corpse_drops(serial);
    if (entry.serial == serial)
    {
        // The attachment goes away with the body.
        entry.attached = undefined;
        cleanup_released_weapon(entry);
    }
}

function clear_models()
{
    restore_recorded_drops();
    foreach (entry in level.mvm_corpse_entries)
    {
        detach_weapon(entry);
        cleanup_released_weapon(entry);
        entry.released = false;
        entry.release_attempted = false;
    }
    SetDvar("mvm_corpse_models", 0);
}

function playback_manager()
{
    // Let finalization/loading return before inspecting the first snapshot.
    // A frame-end barrier alone can be re-entered at the same paused demo tick.
    waitrealtime(0.05);
    previous_enabled = false;
    previous_time = GetTime();
    last_update = previous_time;
    for (;;)
    {
        // Stock callbacks are assigned during level pre-init, before our main.
        // Preserve their full behavior and chain only the entity-spawn event.
        if (!IsDefined(level.mvm_original_entity_spawned) && IsDefined(level.callbackEntitySpawned))
        {
            level.mvm_original_entity_spawned = level.callbackEntitySpawned;
            level.callbackEntitySpawned = &on_entity_spawned;
            SetDvar("mvm_corpse_stage", "watching");
            SetDvar("mvm_corpse_ready", 1);
        }
        enabled = IsDemoPlaying() && GetDvarInt("mvm_holdgun", 0) != 0;
        now = GetTime();
        if (now < previous_time || now - previous_time > 1000)
        {
            clear_models();
            level.mvm_corpse_entries = [];
            level.mvm_corpse_refresh = true;
            level.mvm_seed_players = true;
            level.mvm_seed_attempts = 0;
        }
        if (!enabled && previous_enabled)
            clear_models();
        if (enabled && !previous_enabled)
        {
            level.mvm_seed_players = true;
            level.mvm_seed_attempts = 0;
        }
        // Linked models follow the hand in the engine. No per-player/frame
        // position loops are needed. A paused toggle still applies immediately.
        if (enabled && (!previous_enabled || level.mvm_corpse_refresh || level.mvm_drop_refresh || level.mvm_seed_players || now - last_update >= 50))
        {
            if (level.mvm_seed_players && IsDefined(level.mvm_original_entity_spawned))
            {
                players = GetPlayers(0);
                foreach (player in players)
                {
                    player on_player_spawned(0);
                    player capture_corpse(0);
                }
                level.mvm_seed_attempts++;
                // Initial client callbacks can precede the first snapshot.
                // Retry only this bounded bootstrap, never normal playback.
                if (players.size > 0 || level.mvm_seed_attempts >= 30)
                    level.mvm_seed_players = false;
            }
            count = 0;
            foreach (entry in level.mvm_corpse_entries)
            {
                if (!IsDefined(entry.corpse) || !IsDefined(entry.weapon))
                    continue;
                if (!IsDefined(entry.attached) && !entry.released)
                {
                    if (!entry.corpse HasDObj(0))
                        continue;
                    if (!IsDefined(entry.corpse GetTagOrigin("tag_weapon_right")))
                        continue;
                    attach_weapon(entry);
                    if (!IsDefined(entry.watched_serial) || entry.watched_serial != entry.serial)
                    {
                        entry.watched_serial = entry.serial;
                        level thread delete_with_corpse(entry.corpse, entry.serial, entry);
                    }
                    level.mvm_drop_refresh = true;
                }
                else if (IsDefined(entry.attached) && entry.corpse HasDObj(0) && !(entry.corpse IsAttached(entry.attached, "tag_weapon_right")))
                {
                    // A rebuilt body model (for example after a seek) loses attachments.
                    entry.corpse Attach(entry.attached, "tag_weapon_right");
                }
                if (IsDefined(entry.attached))
                {
                    count++;
                    if (GetDvarInt("mvm_releasegun", 0) == 0)
                        continue;
                    position = entry.corpse GetTagOrigin("tag_weapon_right");
                    angles = entry.corpse GetTagAngles("tag_weapon_right");
                    delay = GetDvarFloat("mvm_releasegun_delay", 0.65);
                    if (!entry.release_attempted && GetDvarInt("mvm_releasegun", 0) != 0 && now - entry.born >= delay * 1000)
                    {
                        entry.release_attempted = true;
                        velocity = (0, 0, 0);
                        if (IsDefined(entry.previous_position) && now > entry.previous_time && now - entry.previous_time <= 250)
                            velocity = (position - entry.previous_position) * (1000.0 / (now - entry.previous_time));
                        dropped = CreateDynEntAndLaunch(0, entry.weapon.worldModel, position, angles, position - (0, 0, 4), velocity);
                        if (IsDefined(dropped))
                        {
                            entry.dropped = dropped;
                            detach_weapon(entry);
                            entry.released = true;
                            count--;
                        }
                    }
                    entry.previous_position = position;
                    entry.previous_time = now;
                }
            }
            SetDvar("mvm_corpse_models", count);
            if (level.mvm_drop_refresh)
                refresh_recorded_drops();
            level.mvm_corpse_refresh = false;
            last_update = now;
        }
        previous_time = now;
        previous_enabled = enabled;
        // A strictly positive real-time wait yields even when demo time is
        // paused/zero. Linked models follow the hand in the engine between
        // updates; this manager only needs 20 Hz for toggles and release logic.
        waitrealtime(0.05);
    }
}
