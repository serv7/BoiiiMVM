#using scripts\shared\callbacks_shared;
#using scripts\shared\system_shared;
#using scripts\shared\util_shared;
#namespace mvm_corpse;

// Hold gun for films. When a player dies the server replaces the player with a body
// entity in the same snapshot (ClonePlayer), and that body plays the death animation.
// From that moment our own copy of the dead player's gun follows the body's hand; just
// above the floor it is let go and falls, bounces and comes to rest.
//
// The gun is the one the player was last seen holding. It is NOT taken from the items
// the game drops at a death: films recorded with the bot tools have no dropped gun
// (mvm_weapon_drops 0), and the item that does appear there is the Scavenger bag.
// A dropped gun that does exist in the film is hidden while ours is shown.
//
// Only deaths seen from the start get a held gun. A body found any other way (film
// start, after a seek) is left alone, so a gun never jumps into a hand.
//
// RULE FOR THIS FILE: no constant vectors such as (0, 0, 4), not even (0, 0, 0).
// BOIII's script compiler embeds them in the script and they corrupt the vector math
// around them (botmod.gsc has the same warning). Every earlier version of this script
// broke on that: thousands of errors per death in script.log and a crash. Build vectors
// from components at run time: (v[0], v[1], v[2] + 4).

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
    level.mvm_holds = [];
    level.mvm_hold_serial = 0;
    level.mvm_guns = [];   // last gun seen in each player's hands, by entity number
    level.mvm_spots = [];  // where each player was last seen
    level.mvm_deaths = []; // players that just went away, waiting for their body
    level.mvm_items = [];  // dropped guns that just appeared
    level.mvm_quiet_until = 0;
    level.mvm_jump_pending = true;
    callback::on_spawned(&on_player_spawned);
    level thread playback_manager();
    level thread watch_demo_jumps("demo_jump");
    level thread watch_demo_jumps("demo_jump0");
}

// Diagnostics: the last few events are kept in the dvar mvm_corpse_trace. The theater
// plugin copies new ones into MVM\theater.log ("Hold gun: ...").
function log_event(text)
{
    if (!IsDefined(level.mvm_events))
    {
        level.mvm_events = [];
        level.mvm_event_count = 0;
    }
    level.mvm_event_count++;
    level.mvm_events[level.mvm_events.size] = level.mvm_event_count + " t" + GetTime() + " " + text;
    if (level.mvm_events.size > 10)
        ArrayRemoveIndex(level.mvm_events, 0);
    joined = "";
    foreach (entry_text in level.mvm_events)
        joined = joined + entry_text + " | ";
    SetDvar("mvm_corpse_trace", joined);
}

function holding_enabled()
{
    return IsDemoPlaying() && GetDvarInt("mvm_holdgun", 0) != 0;
}

function on_player_spawned(localClientNum)
{
    if (!self IsPlayer())
        return;
    if (!IsDefined(self.mvm_shutdown_registered))
    {
        self.mvm_shutdown_registered = true;
        self callback::on_shutdown(&on_player_shutdown);
    }
}

// A player entity went away. At a death its body appears in the same snapshot.
function on_player_shutdown(localClientNum)
{
    if (!self IsPlayer())
        return;
    if (!holding_enabled())
        return;
    check_time_jump();
    now = GetTime();
    if (now < level.mvm_quiet_until)
        return;
    id = self GetEntityNumber();
    gun = level.mvm_guns[id];
    spot = level.mvm_spots[id];
    if (!IsDefined(gun) || !IsDefined(spot))
    {
        log_event("p" + id + " gone, gun unknown");
        return;
    }
    body = self GetPlayerCorpse();
    if (IsDefined(body) && body != self && !(body IsPlayer()) && !body_is_held(body))
    {
        log_event("p" + id + " gone, body e" + body GetEntityNumber());
        start_hold(body, gun, id, now);
        return;
    }
    // The body may already have been seen without an owner, or may come a moment later.
    body = fresh_body_near(spot, now);
    if (IsDefined(body))
    {
        log_event("p" + id + " gone, near body e" + body GetEntityNumber());
        start_hold(body, gun, id, now);
        return;
    }
    death = SpawnStruct();
    death.id = id;
    death.gun = gun;
    death.spot = spot;
    death.time = now;
    death.used = false;
    level.mvm_deaths[level.mvm_deaths.size] = death;
    log_event("p" + id + " gone, waiting for body");
}

function fresh_body_near(spot, now)
{
    found = undefined;
    best = 150 * 150;
    foreach (body in GetEntArrayByType(0, 2))
    {
        if (!IsDefined(body.mvm_fresh) || now - body.mvm_fresh > 500 || now < body.mvm_fresh)
            continue;
        if (body_is_held(body))
            continue;
        gap = DistanceSquared(body.origin, spot);
        if (gap < best)
        {
            best = gap;
            found = body;
        }
    }
    return found;
}

function on_entity_spawned(localClientNum)
{
    if (IsDefined(level.mvm_original_entity_spawned))
        self [[level.mvm_original_entity_spawned]](localClientNum);
    if (self IsPlayer())
        self on_player_spawned(localClientNum);
    if (!holding_enabled())
        return;
    check_time_jump();
    // Types are asked from the engine. Reading .type on any entity caused script errors.
    foreach (body in GetEntArrayByType(0, 2))
    {
        if (body == self)
        {
            if (!IsDefined(body.mvm_seen))
                claim_body(body);
            return;
        }
    }
    foreach (thing in GetEntArrayByType(0, 3))
    {
        if (thing == self)
        {
            if (!IsDefined(thing.mvm_seen))
                claim_item(thing);
            return;
        }
    }
}

function watch_demo_jumps(signal)
{
    for (;;)
    {
        level waittill(signal);
        level.mvm_jump_pending = true;
    }
}

// Seeks and rewinds: drop every hold, and for a moment treat new bodies and items as
// ones that were already lying on the map.
function check_time_jump()
{
    now = GetTime();
    jumped = level.mvm_jump_pending;
    if (IsDefined(level.mvm_last_time))
    {
        if (now < level.mvm_last_time)
            jumped = true;
        if (now - level.mvm_last_time > 3000)
            jumped = true;
    }
    before = level.mvm_last_time;
    level.mvm_last_time = now;
    if (!jumped)
        return;
    level.mvm_jump_pending = false;
    level.mvm_quiet_until = now + 300;
    level.mvm_deaths = [];
    level.mvm_items = [];
    clear_all_holds();
    if (IsDefined(before))
        log_event("jump " + before + " -> " + now);
}

// A body entity seen for the first time.
function claim_body(body)
{
    body.mvm_seen = true;
    now = GetTime();
    if (now < level.mvm_quiet_until)
        return;
    body.mvm_fresh = now;
    if (body_is_held(body))
        return;
    // A player that went away a moment ago, right here.
    found = undefined;
    best = 150 * 150;
    foreach (death in level.mvm_deaths)
    {
        if (death.used || now - death.time > 500 || now < death.time)
            continue;
        gap = DistanceSquared(death.spot, body.origin);
        if (gap < best)
        {
            best = gap;
            found = death;
        }
    }
    if (IsDefined(found))
    {
        found.used = true;
        log_event("body e" + body GetEntityNumber() + " for p" + found.id);
        start_hold(body, found.gun, found.id, now);
        return;
    }
    // The watched player's own entity stays when it dies.
    foreach (player in GetPlayers(0))
    {
        owned = player GetPlayerCorpse();
        if (IsDefined(owned) && owned == body)
        {
            id = player GetEntityNumber();
            gun = level.mvm_guns[id];
            if (IsDefined(gun))
            {
                log_event("body e" + body GetEntityNumber() + " of p" + id);
                start_hold(body, gun, id, now);
            }
            return;
        }
    }
    log_event("body e" + body GetEntityNumber() + " no owner yet");
}

// A dropped item seen for the first time. A gun dropped at a death is hidden while our
// own copy is shown. The Scavenger bag is not a gun.
function claim_item(thing)
{
    thing.mvm_seen = true;
    now = GetTime();
    if (now < level.mvm_quiet_until)
        return;
    if (!IsDefined(thing.weapon) || !IsDefined(thing.weapon.worldModel) || thing.weapon.worldModel == "")
        return;
    if (IsDefined(thing.weapon.name) && IsSubStr(thing.weapon.name, "scavenger"))
        return;
    entry = SpawnStruct();
    entry.thing = thing;
    entry.time = now;
    entry.used = false;
    level.mvm_items[level.mvm_items.size] = entry;
    match_items();
}

function hold_for_item(entry)
{
    foreach (hold in level.mvm_holds)
    {
        if (hold.done || IsDefined(hold.item))
            continue;
        if (hold.born - entry.time > 500 || entry.time - hold.born > 500)
            continue;
        if (DistanceSquared(entry.thing.origin, hold.spot) > 150 * 150)
            continue;
        return hold;
    }
}

function match_items()
{
    foreach (entry in level.mvm_items)
    {
        if (entry.used || !IsDefined(entry.thing))
            continue;
        hold = hold_for_item(entry);
        if (!IsDefined(hold))
            continue;
        entry.used = true;
        hold.item = entry.thing;
        entry.thing Hide();
        log_event("s" + hold.serial + " hides dropped gun e" + entry.thing GetEntityNumber());
    }
}

function body_is_held(body)
{
    foreach (hold in level.mvm_holds)
    {
        if (!hold.done && IsDefined(hold.body) && hold.body == body)
            return true;
    }
    return false;
}

function start_hold(body, gun, id, now)
{
    spot = body.origin;
    zero_vec = spot - spot; // a zero vector without a constant
    level.mvm_hold_serial++;
    hold = SpawnStruct();
    hold.serial = level.mvm_hold_serial;
    hold.body = body;
    hold.owner = id;
    hold.born = now;
    hold.spot = spot;
    hold.done = false;
    hold.seen_hand = false;
    hold.let_go = false;
    hold.released = false;
    hold.resting = false;
    hold.settled = false;
    hold.bounces = 0;
    hold.height = 1000;
    hold.pos = spot;
    hold.ang = zero_vec;
    hold.vel = zero_vec;
    hold.spin = zero_vec;
    hold.sample_pos = spot;
    hold.sample_time = now;
    hold.last_time = now;
    hold.model = Spawn(0, spot, "script_model");
    hold.model SetModel(gun.worldModel);
    // The weapon model has collision; where it met the ragdoll the body was thrown across
    // the map. NotSolid is in the client method table.
    hold.model NotSolid();
    // Shown once the body's hand is known.
    hold.model Hide();
    level.mvm_holds[level.mvm_holds.size] = hold;
    log_event("s" + hold.serial + " hold p" + id + " body e" + body GetEntityNumber() + " " + gun.worldModel);
    body thread watch_body(hold);
    level thread run_hold(hold);
    match_items();
}

function watch_body(hold)
{
    self waittill("entityshutdown");
    if (!hold.done)
    {
        log_event("s" + hold.serial + " body gone");
        clear_hold(hold);
    }
}

// One hold, every rendered frame. Times are demo times, so pause and slow motion apply.
function run_hold(hold)
{
    while (!hold.done)
    {
        now = GetTime();
        if (IsDefined(hold.item) && !(hold.item IsHidden()))
            hold.item Hide();
        if (!hold.released)
        {
            has_hand = false;
            if (IsDefined(hold.body) && hold.body HasDObj(0))
            {
                hand = hold.body GetTagOrigin("tag_weapon_right");
                if (IsDefined(hand))
                    has_hand = true;
            }
            if (has_hand)
            {
                turn = hold.body GetTagAngles("tag_weapon_right");
                hold.model.origin = hand;
                hold.model.angles = turn;
                hold.pos = hand;
                hold.ang = turn;
                if (!hold.seen_hand)
                {
                    hold.seen_hand = true;
                    hold.sample_pos = hand;
                    hold.sample_time = now;
                    hold.model Show();
                    log_event("s" + hold.serial + " in hand");
                }
                if (now - hold.sample_time >= 30)
                {
                    hold.vel = (hand - hold.sample_pos) * (1000.0 / (now - hold.sample_time));
                    hold.sample_pos = hand;
                    hold.sample_time = now;
                }
                above = (hand[0], hand[1], hand[2] + 4);
                lower = (hand[0], hand[1], hand[2] - 128);
                ground = BulletTrace(above, lower, false, hold.body);
                height = 1000;
                if (IsDefined(ground) && IsDefined(ground["fraction"]) && ground["fraction"] < 1)
                {
                    floor_spot = ground["position"];
                    height = hand[2] - floor_spot[2];
                }
                hold.height = height;
                age = now - hold.born;
                moving = hold.vel;
                // About to hit the floor: let go with the hand's motion, the gun carries on and bounces.
                if (moving[2] < -40 && height < 22)
                    hold.let_go = true;
                // The hand came to rest (arm lowered slowly, or resting above the floor).
                if (Length(moving) < 30)
                {
                    if (!IsDefined(hold.still_since))
                        hold.still_since = now;
                    if (now - hold.still_since >= 250 && age >= 500)
                        hold.let_go = true;
                }
                else
                    hold.still_since = undefined;
                if (age >= 3500)
                    hold.let_go = true;
                if (GetDvarInt("mvm_releasegun", 0) != 0 && age >= GetDvarFloat("mvm_releasegun_delay", 0.65) * 1000)
                    hold.let_go = true;
            }
            else if (now - hold.born > 3500 || !IsDefined(hold.body))
            {
                // No hand to follow. A gun that was never shown just goes away.
                if (!hold.seen_hand)
                {
                    log_event("s" + hold.serial + " no hand found");
                    clear_hold(hold);
                }
                else
                    hold.let_go = true;
            }
            if (hold.let_go && !hold.done)
            {
                hold.released = true;
                hold.release_time = now;
                push = hold.vel;
                if (Length(push) > 300)
                    push = VectorNormalize(push) * 300;
                hold.vel = push;
                hold.spin = (RandomFloatRange(-150, 150), RandomFloatRange(-90, 90), RandomFloatRange(-360, 360));
                // A hand that ended up slightly inside the floor would start the gun under it.
                start_pos = hold.pos;
                upper = (start_pos[0], start_pos[1], start_pos[2] + 24);
                lift = BulletTrace(upper, start_pos, false, hold.model);
                if (IsDefined(lift) && IsDefined(lift["fraction"]) && lift["fraction"] < 1)
                {
                    hit_pos = lift["position"];
                    hold.pos = (hit_pos[0], hit_pos[1], hit_pos[2] + 2);
                }
                log_event("s" + hold.serial + " drop h" + Int(hold.height) + " vz" + Int(push[2]) + " age" + (now - hold.born));
            }
        }
        else if (!hold.settled)
        {
            dt = (now - hold.last_time) / 1000.0;
            if (dt > 0)
            {
                if (dt > 0.1)
                    dt = 0.1;
                if (!hold.resting)
                {
                    motion = hold.vel;
                    motion = (motion[0], motion[1], motion[2] - 800 * dt);
                    next_pos = hold.pos + motion * dt;
                    contact = BulletTrace(hold.pos, next_pos, false, hold.model);
                    if (IsDefined(contact) && IsDefined(contact["fraction"]) && contact["fraction"] < 1)
                    {
                        surface = contact["normal"];
                        inward = VectorDot(motion, surface);
                        if (inward < 0)
                            motion = (motion - surface * (1.4 * inward)) * 0.6;
                        hold.pos = contact["position"] + surface * 1.5;
                        hold.spin = hold.spin * 0.45;
                        hold.bounces++;
                        if (Length(motion) < 45 || hold.bounces >= 4)
                            hold.resting = true;
                    }
                    else
                        hold.pos = next_pos;
                    hold.vel = motion;
                    hold.ang = hold.ang + hold.spin * dt;
                    if (now - hold.release_time > 4000)
                        hold.resting = true;
                    if (hold.resting)
                    {
                        // Lie flat on its side where it stopped.
                        lying = hold.ang;
                        roll = 90;
                        if (AngleClamp180(lying[2]) < 0)
                            roll = -90;
                        hold.rest_time = now;
                        hold.rest_from_pos = hold.pos;
                        hold.rest_from_ang = lying;
                        hold.rest_ang = (0, lying[1], roll);
                        hold.rest_pos = hold.pos;
                        start_pos = hold.pos;
                        upper = (start_pos[0], start_pos[1], start_pos[2] + 8);
                        lower = (start_pos[0], start_pos[1], start_pos[2] - 64);
                        below = BulletTrace(upper, lower, false, hold.model);
                        if (IsDefined(below) && IsDefined(below["fraction"]) && below["fraction"] < 1)
                        {
                            hit_pos = below["position"];
                            hold.rest_pos = (hit_pos[0], hit_pos[1], hit_pos[2] + 1.5);
                        }
                        log_event("s" + hold.serial + " rest after " + hold.bounces + " bounces");
                    }
                }
                else
                {
                    blend = (now - hold.rest_time) / 180.0;
                    if (blend >= 1)
                    {
                        blend = 1;
                        hold.settled = true;
                    }
                    turn = hold.rest_ang - hold.rest_from_ang;
                    turn = (AngleClamp180(turn[0]), AngleClamp180(turn[1]), AngleClamp180(turn[2]));
                    hold.ang = hold.rest_from_ang + turn * blend;
                    hold.pos = hold.rest_from_pos + (hold.rest_pos - hold.rest_from_pos) * blend;
                }
                hold.model.origin = hold.pos;
                hold.model.angles = hold.ang;
            }
        }
        hold.last_time = now;
        // A tiny real-time wait resumes on the next rendered frame, also while the film is
        // paused or in slow motion. A gun at rest only needs the item kept hidden.
        if (hold.settled)
            waitrealtime(0.1);
        else
            waitrealtime(0.001);
    }
}

function clear_hold(hold)
{
    if (hold.done)
        return;
    hold.done = true;
    if (IsDefined(hold.model))
        hold.model Delete();
    if (IsDefined(hold.item) && hold.item IsHidden())
        hold.item Show();
}

function clear_all_holds()
{
    foreach (hold in level.mvm_holds)
        clear_hold(hold);
    level.mvm_holds = [];
}

function playback_manager()
{
    // Let finalization/loading return before inspecting the first snapshot.
    waitrealtime(0.05);
    was_enabled = false;
    for (;;)
    {
        // Stock callbacks are assigned during level pre-init, before our main.
        // Preserve their full behavior and chain only this event.
        if (!IsDefined(level.mvm_original_entity_spawned) && IsDefined(level.callbackEntitySpawned))
        {
            level.mvm_original_entity_spawned = level.callbackEntitySpawned;
            level.callbackEntitySpawned = &on_entity_spawned;
            SetDvar("mvm_corpse_stage", "watching");
            SetDvar("mvm_corpse_ready", 1);
            foreach (player in GetPlayers(0))
                player on_player_spawned(0);
            log_event("ready");
        }
        enabled = holding_enabled();
        if (enabled)
        {
            check_time_jump();
            now = GetTime();
            // What each player holds and where it is, for the moment it dies.
            foreach (player in GetPlayers(0))
            {
                id = player GetEntityNumber();
                gun = player.weapon;
                if (IsDefined(gun) && IsDefined(gun.worldModel) && gun.worldModel != "")
                    level.mvm_guns[id] = gun;
                level.mvm_spots[id] = player.origin;
            }
            // Fallback in case a spawn was not reported: bodies and items not seen yet.
            foreach (body in GetEntArrayByType(0, 2))
            {
                if (!IsDefined(body.mvm_seen))
                    claim_body(body);
            }
            foreach (thing in GetEntArrayByType(0, 3))
            {
                if (!IsDefined(thing.mvm_seen))
                    claim_item(thing);
            }
            // Forget deaths and dropped guns that found no partner.
            waiting = [];
            foreach (death in level.mvm_deaths)
            {
                if (!death.used && now >= death.time && now - death.time <= 1000)
                    waiting[waiting.size] = death;
            }
            level.mvm_deaths = waiting;
            recent = [];
            foreach (entry in level.mvm_items)
            {
                if (!entry.used && now >= entry.time && now - entry.time <= 1000)
                    recent[recent.size] = entry;
            }
            level.mvm_items = recent;
        }
        else if (was_enabled)
            clear_all_holds();
        active = [];
        foreach (hold in level.mvm_holds)
        {
            if (!hold.done)
                active[active.size] = hold;
        }
        // Keep the number of guns on the floor bounded.
        while (active.size > 12)
        {
            clear_hold(active[0]);
            ArrayRemoveIndex(active, 0);
        }
        level.mvm_holds = active;
        SetDvar("mvm_corpse_models", active.size);
        was_enabled = enabled;
        waitrealtime(0.05);
    }
}
