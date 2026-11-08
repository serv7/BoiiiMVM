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

-- END GAME for the host of a live multiplayer match. BOIII's pause menu only
-- offers Restart and Main Menu there; leaving that way never finishes the match,
-- so the film is not finalised. Stock BO3 ends the match with the "endround"
-- menu response (mp/gametypes/_menus.gsc -> globallogic::forceEnd).
local function isLocalGameHost()
  if game and game.ishost then
    return game.ishost()
  end
  local connectedToDedicated = false
  pcall(function()
    connectedToDedicated = Dvar.cl_connected_to_dedi and Dvar.cl_connected_to_dedi:get()
  end)
  return Engine.IsLobbyHost(Enum.LobbyType.LOBBY_TYPE_GAME) and not connectedToDedicated
end

local function wantsEndGame()
  return CoD ~= nil and Engine.IsInGame() and not Engine.IsDemoPlaying()
    and Engine.CurrentSessionMode() == Enum.eModes.MODE_MULTIPLAYER and isLocalGameHost()
end

local function endGame(self, element, controller, param, menu)
  Engine.SendMenuResponse(controller, "popup_leavegame", "endround")
  if type(StartMenuGoBack_ListElement) == "function" then
    StartMenuGoBack_ListElement(self, element, controller, param, menu)
  end
end

local function popup(name)
  return function(self, element, controller, param, menu)
    if menu and menu.openPopup then
      menu:openPopup(name, controller)
    else
      OpenPopup(self, name, controller)
    end
  end
end

-- Mirrors the multiplayer host rows of BOIII's party/datasources_start_menu_game_options.lua,
-- with END GAME added. Other modes, films and non-hosts keep BOIII's own list.
local function hostMultiplayerOptions(controller)
  local options = {}
  if Engine.Team(controller, "name") ~= "TEAM_SPECTATOR" and Engine.GetGametypeSetting("disableClassSelection") ~= 1 then
    table.insert(options, { models = { displayText = "MPUI_CHOOSE_CLASS_BUTTON_CAPS", action = ChooseClass } })
  end
  if
    not Engine.IsVisibilityBitSet(controller, Enum.UIVisibilityBit.BIT_ROUND_END_KILLCAM)
    and not Engine.IsVisibilityBitSet(controller, Enum.UIVisibilityBit.BIT_FINAL_KILLCAM)
    and CoD.IsTeamChangeAllowed()
  then
    table.insert(options, { models = { displayText = "MPUI_CHANGE_TEAM_BUTTON_CAPS", action = ChooseTeam } })
  end
  local friendsCanJoin = Engine.DvarBool(controller, "friends_open")
  table.insert(options, {
    models = {
      displayText = friendsCanJoin and "CLOSE FRIEND JOINING" or "OPEN TO FRIENDS",
      action = function(self, element, actionController, param, menu)
        Engine.Exec(actionController, "friends_open")
        StartMenuGoBack_ListElement(self, element, actionController, param, menu)
      end,
    },
  })
  table.insert(options, { models = { displayText = "GAME TWEAKS", action = popup("BoiiiGameTweaksMenu") } })
  table.insert(options, { models = { displayText = "KICK PLAYER", action = popup("BoiiiKickPlayersMenu") } })
  table.insert(options, { models = { displayText = "END GAME", action = endGame } })
  table.insert(options, { models = { displayText = "RESTART GAME", action = RestartGame } })
  table.insert(options, { models = { displayText = "MAIN MENU", action = QuitGame_MP } })
  table.insert(options, { models = { displayText = "QUIT TO DESKTOP", action = OpenPCQuit } })
  return options
end

local hostSource = nil
local boiiiSource = nil

local function selectGameOptionsSource()
  if not DataSources or type(ListHelper_SetupDataSource) ~= "function" then
    return
  end
  local current = BoiiiStartMenuGameOptions or DataSources.StartMenuGameOptions
  if current ~= nil and current ~= hostSource then
    boiiiSource = current
  end
  if wantsEndGame() then
    if not hostSource then
      hostSource = ListHelper_SetupDataSource("MvmHostStartMenuGameOptions", hostMultiplayerOptions, true)
    end
    DataSources.StartMenuGameOptions = hostSource
    BoiiiStartMenuGameOptions = hostSource
  elseif boiiiSource then
    DataSources.StartMenuGameOptions = boiiiSource
    BoiiiStartMenuGameOptions = boiiiSource
  end
end

local function patchCurrentSources()
  selectGameOptionsSource()
  if DataSources then
    patchDataSource(DataSources.StartMenuGameOptions)
  end
  -- The existing party menu restores this reference during map transitions.
  patchDataSource(BoiiiStartMenuGameOptions)
end

-- BOIII sizes the pause list for 8 rows; the host list has 9 with END GAME.
local function fitHostOptions(instance)
  if type(instance) ~= "table" or not instance.buttonList or not hostSource
    or DataSources.StartMenuGameOptions ~= hostSource then
    return
  end
  instance.buttonList:setTopBottom(true, false, 5, 304)
  instance.buttonList:setVerticalCount(9)
  if instance.StartMenuConnectionMeterContainer0 then
    instance.StartMenuConnectionMeterContainer0:setTopBottom(true, false, 313, 458)
  end
end

local function patchConstructor(owner, key, marker, before, after)
  if type(owner) ~= "table" or type(owner[key]) ~= "function" then
    return
  end
  if owner[key] == owner[marker] then
    return
  end
  local original = owner[key]
  local constructor = function(...)
    if before then
      before()
    end
    -- BO3 can reload its default CoD helper after addon initialization. Apply
    -- the permission at menu construction and datasource preparation as well.
    restoreTeamChangePermission()
    patchCurrentSources()
    if not after then
      return original(...)
    end
    -- Keep every value the original constructor returns.
    local results = { n = 0 }
    local function collect(...)
      results.n = select("#", ...)
      for index = 1, results.n do
        results[index] = select(index, ...)
      end
    end
    collect(original(...))
    after(results[1])
    return (table.unpack or unpack)(results, 1, results.n)
  end
  owner[key] = constructor
  owner[marker] = constructor
end

local function patchGameOptionsWidget()
  -- Re-wrap if the party script wrapped after us, so the resize runs last.
  if CoD then
    patchConstructor(CoD.StartMenu_GameOptions, "new", "mvmTeamChangeNew", nil, fitHostOptions)
  end
end

restoreTeamChangePermission()
patchCurrentSources()
patchGameOptionsWidget()
if LUI and LUI.createMenu then
  -- The pause menu builds its game-options widget inside this constructor.
  patchConstructor(LUI.createMenu, "StartMenu", "mvmTeamChangeStartMenu", patchGameOptionsWidget)
end
