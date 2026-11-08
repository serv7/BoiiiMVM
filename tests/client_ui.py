"""Offline Lua 5.1 integration checks; never launches or controls BOIII.

Install lupa in build/lua-test-runtime, or in the test Python environment.
Pass --party-directory to include the installed BOIII party scripts in checks.
"""
import argparse
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "build/lua-test-runtime"))
from lupa.lua51 import LuaRuntime

SOURCE = (ROOT / "assets/client-ui/mvm_client_preferences/__init__.lua").read_text()
parser = argparse.ArgumentParser()
parser.add_argument("--party-directory", type=Path)
args, remaining = parser.parse_known_args()

FIXTURE = r'''
inGame = true; demo = false; mode = 1; allowTeams = 1; killcam = false
safe = {safeArea_horizontal = 1, safeArea_vertical = 1}
settingsWrites = 0; nativeConstructors = 0; chosenTeams = 0
host = false; responses = {}; wentBack = 0; visibleRows = 8
Enum = {eModes = {MODE_MULTIPLAYER=1, MODE_ZOMBIES=2, MODE_CAMPAIGN=3},
        LobbyType={LOBBY_TYPE_GAME=1}, UIVisibilityBit={BIT_ROUND_END_KILLCAM=1, BIT_FINAL_KILLCAM=2}}
Engine = {
  SetDvar=function(name,value) safe[name]=value; settingsWrites=settingsWrites+1 end,
  IsInGame=function() return inGame end, IsDemoPlaying=function() return demo end,
  CurrentSessionMode=function() return mode end,
  GetGametypeSetting=function(name) if name=='allowInGameTeamChange' then return allowTeams end; return 0 end,
  IsVisibilityBitSet=function() return killcam end,
  Team=function() return 'TEAM_ALLIES' end, IsLobbyHost=function() return host end,
  DvarBool=function() return false end,
  SendMenuResponse=function(controller,menu,response) table.insert(responses,menu..':'..response) end,
  GetLobbyUIScreen=function() return 0 end,
  Exec=function() error('The addon must not issue gameplay commands') end,
}
LobbyData={UITargetFromId=function() return {maxClients=18} end}
CoD = {IsTeamChangeAllowed=function() return false end}
ChooseTeam=function() chosenTeams=chosenTeams+1 end
ChooseClass=function() end; QuitGame_MP=function() end; OpenPCQuit=function() end
RestartGame=function() end; OpenPopup=function() end
StartMenuGoBack_ListElement=function() wentBack=wentBack+1 end
-- Film-menu helpers used by BOIII's party generator.
IsDemoRestrictedBasicMode=function() return false end
StartMenuUploadClip=function() end; StartMenuJumpToStart=function() end; StartMenuEndDemo=function() end
Engine.IsDemoHighlightReelMode=function() return false end; Engine.IsDemoClipPlaying=function() return false end
Engine.ToUpper=function(text) return text end; Engine.Localize=function(text) return text end
Engine.GetDemoSegmentCount=function() return 0 end
-- Campaign-menu helpers used by BOIII's party generator.
Engine.GetModelForController=function() return {} end; Engine.GetPrimaryController=function() return 0 end
CoD.SafeGetModelValue=function() return 0 end
RestartMission=function() end; OpenDifficultySelect=function() end; SaveAndQuitGame=function() end; QuitGame=function() end
function ListHelper_SetupDataSource(name, build)
  return {prepare=function(controller,list,filter) return build(controller),'prepared',filter end}
end
DataSources={StartMenuGameOptions=ListHelper_SetupDataSource('StartMenuGameOptions',function()
  local rows={{models={displayText='other'}}}
  if CoD.IsTeamChangeAllowed() then
    table.insert(rows,{models={displayText='MPUI_CHANGE_TEAM_BUTTON_CAPS',action=ChooseTeam}})
  end
  return rows
end)}
function newWidget(menu,controller)
  nativeConstructors=nativeConstructors+1
  local rows=DataSources.StartMenuGameOptions.prepare(controller,{},'filter-token')
  return {rows=rows,buttonList={setTopBottom=function() end,setVerticalCount=function(self,n) visibleRows=n end}},'widget-token'
end
CoD.StartMenu_GameOptions={new=newWidget}
LUI={roots={},createMenu={StartMenu=function(...)
  local instance=CoD.StartMenu_GameOptions.new(...)
  return instance,'menu-token'
end}}
function require() end
function rowText(rows,text)
  for _,row in ipairs(rows) do if row.models.displayText==text then return row end end
end
function currentMenuRows()
  local menu=LUI.createMenu.StartMenu({},0)
  return menu.rows
end
function hasTeam(rows)
  for _,row in ipairs(rows) do if row.models.displayText=='MPUI_CHANGE_TEAM_BUTTON_CAPS' then return true end end
  return false
end
function currentMenuHasTeam()
  local menu,token=LUI.createMenu.StartMenu({},0)
  assert(token=='menu-token')
  return hasTeam(menu.rows)
end
'''


class ClientUiChecks(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute(FIXTURE)
        if args.party_directory:
            for name in ("datasources_start_menu_game_options.lua", "__init__.lua"):
                self.lua.execute((args.party_directory / name).read_text())
        self.g = self.lua.globals()

    def install(self):
        self.lua.execute(SOURCE)

    def reset_native_permission(self):
        self.lua.execute("CoD.IsTeamChangeAllowed=function() return false end")

    def test_public_menu_after_native_helper_is_reloaded(self):
        self.reset_native_permission()
        self.assertFalse(self.lua.eval("hasTeam(DataSources.StartMenuGameOptions.prepare(0))"))
        self.install()
        self.reset_native_permission()
        self.assertTrue(self.g.currentMenuHasTeam())
        self.assertEqual(self.g.nativeConstructors, 1)
        self.lua.execute("for _,row in ipairs(DataSources.StartMenuGameOptions.prepare(0)) do if row.models.displayText=='MPUI_CHANGE_TEAM_BUTTON_CAPS' then assert(row.models.action==ChooseTeam); row.models.action() end end")
        self.assertEqual(self.g.chosenTeams, 1)

    def test_source_refresh_restores_permission_without_rebuilding_menu(self):
        self.install()
        self.reset_native_permission()
        self.assertTrue(self.lua.eval("hasTeam(DataSources.StartMenuGameOptions.prepare(0))"))

    def test_server_disallow_is_respected(self):
        self.install()
        self.g.allowTeams = 0
        self.assertFalse(self.g.currentMenuHasTeam())

    def test_killcam_filters_in_actual_party_generator_are_preserved(self):
        if not args.party_directory:
            self.skipTest("Actual party source not provided")
        self.install()
        self.g.killcam = True
        self.assertFalse(self.g.currentMenuHasTeam())

    def test_safe_area_is_reapplied_on_each_ui_load_not_each_menu(self):
        self.install()
        self.assertAlmostEqual(self.g.safe.safeArea_horizontal, .88)
        self.assertAlmostEqual(self.g.safe.safeArea_vertical, .88)
        writes = self.g.settingsWrites
        self.g.currentMenuHasTeam()
        self.assertEqual(self.g.settingsWrites, writes)
        self.lua.execute("safe.safeArea_horizontal=1; safe.safeArea_vertical=1")
        self.install()
        self.assertAlmostEqual(self.g.safe.safeArea_horizontal, .88)
        self.assertAlmostEqual(self.g.safe.safeArea_vertical, .88)

    def test_repeated_loading_does_not_stack_constructor_wrappers(self):
        self.install()
        original = self.lua.eval("tostring(CoD.StartMenu_GameOptions.new)")
        for _ in range(5):
            self.install()
        self.assertEqual(original, self.lua.eval("tostring(CoD.StartMenu_GameOptions.new)"))
        self.assertTrue(self.g.currentMenuHasTeam())
        self.assertEqual(self.g.nativeConstructors, 1)

    def test_non_mp_frontend_and_demo_permissions_are_unchanged(self):
        for mode, demo, in_game in ((2, False, True), (3, False, True), (1, True, True), (1, False, False)):
            self.g.mode, self.g.demo, self.g.inGame = mode, demo, in_game
            self.reset_native_permission()
            self.install()
            self.assertFalse(self.g.CoD.IsTeamChangeAllowed())

    def test_startup_with_no_loaded_menu_widgets_still_sets_safe_area(self):
        self.lua.execute("CoD=nil; DataSources=nil; LUI=nil; BoiiiStartMenuGameOptions=nil")
        self.install()
        self.assertAlmostEqual(self.g.safe.safeArea_horizontal, .88)
        self.assertAlmostEqual(self.g.safe.safeArea_vertical, .88)

    def test_end_game_for_live_match_host_ends_the_round(self):
        self.g.host = True
        self.install()
        rows = self.g.currentMenuRows()
        row = self.g.rowText(rows, "END GAME")
        self.assertIsNotNone(row)
        for text in ("RESTART GAME", "MAIN MENU", "QUIT TO DESKTOP", "GAME TWEAKS", "KICK PLAYER"):
            self.assertIsNotNone(self.g.rowText(rows, text), text)
        self.assertEqual(self.g.visibleRows, 9)
        self.lua.execute("rowText(currentMenuRows(),'END GAME').models.action({},{},0,nil,{})")
        self.assertEqual(list(self.g.responses.values()), ["popup_leavegame:endround"])
        self.assertEqual(self.g.wentBack, 1)

    def test_end_game_keeps_team_change_rule(self):
        self.g.host = True
        self.install()
        self.assertTrue(self.g.hasTeam(self.g.currentMenuRows()))
        self.g.allowTeams = 0
        self.assertFalse(self.g.hasTeam(self.g.currentMenuRows()))

    def test_no_end_game_for_guests_films_or_other_modes(self):
        for host, mode, demo in ((False, 1, False), (True, 1, True), (True, 2, False), (True, 3, False)):
            self.setUp()
            self.g.host, self.g.mode, self.g.demo = host, mode, demo
            self.install()
            self.assertIsNone(self.g.rowText(self.g.currentMenuRows(), "END GAME"), (host, mode, demo))
            self.assertNotEqual(self.g.visibleRows, 9)

    def test_host_list_switches_back_when_no_longer_host(self):
        self.g.host = True
        self.install()
        self.assertIsNotNone(self.g.rowText(self.g.currentMenuRows(), "END GAME"))
        self.g.host = False
        self.assertIsNone(self.g.rowText(self.g.currentMenuRows(), "END GAME"))

    def test_repeated_loading_keeps_one_end_game_row(self):
        self.g.host = True
        for _ in range(4):
            self.install()
        rows = self.g.currentMenuRows()
        self.assertEqual(sum(1 for row in rows.values() if row.models.displayText == "END GAME"), 1)
        self.assertEqual(self.g.nativeConstructors, 1)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)
