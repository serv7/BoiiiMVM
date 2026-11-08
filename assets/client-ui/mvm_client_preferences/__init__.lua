-- Loaded by BOIII from boiii/ui_scripts/ whenever a LUI context is created.
-- These dvars are not archived by the engine, so apply the preferred values
-- after registration on each frontend/in-game UI load, without a polling loop.
Engine.SetDvar("safeArea_horizontal", 0.88)
Engine.SetDvar("safeArea_vertical", 0.88)

local function restoreTeamChangePermission()
  if not CoD or not Engine.IsInGame() or Engine.IsDemoPlaying() then
    return
  end
  if Engine.CurrentSessionMode() ~= Enum.eModes.MODE_MULTIPLAYER then
    return
  end

  -- Public/ranked status is a UI restriction, separate from the server's
  -- allowInGameTeamChange rule. Keep the server rule and normal ChooseTeam flow.
  CoD.IsTeamChangeAllowed = function()
    return Engine.GetGametypeSetting("allowInGameTeamChange") == 1
  end
end

local function patchDataSource(source)
  if type(source) ~= "table" or type(source.prepare) ~= "function" then
    return
  end
  if source.prepare == source.mvmTeamChangePrepare then
    return
  end
  local originalPrepare = source.prepare
  local prepare = function(...)
    restoreTeamChangePermission()
    return originalPrepare(...)
  end
  source.prepare = prepare
  source.mvmTeamChangePrepare = prepare
end

local function patchCurrentSources()
  if DataSources then
    patchDataSource(DataSources.StartMenuGameOptions)
  end
  -- The existing party menu restores this reference during map transitions.
  patchDataSource(BoiiiStartMenuGameOptions)
end

local function patchConstructor(owner, key, marker)
  if type(owner) ~= "table" or type(owner[key]) ~= "function" then
    return
  end
  if owner[key] == owner[marker] then
    return
  end
  local original = owner[key]
  local constructor = function(...)
    -- BO3 can reload its default CoD helper after addon initialization. Apply
    -- the permission at menu construction and datasource preparation as well.
    restoreTeamChangePermission()
    patchCurrentSources()
    return original(...)
  end
  owner[key] = constructor
  owner[marker] = constructor
end

restoreTeamChangePermission()
patchCurrentSources()
if CoD then
  patchConstructor(CoD.StartMenu_GameOptions, "new", "mvmTeamChangeNew")
end
if LUI and LUI.createMenu then
  patchConstructor(LUI.createMenu, "StartMenu", "mvmTeamChangeStartMenu")
end
