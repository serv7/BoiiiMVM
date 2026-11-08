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
Enum = {eModes = {MODE_MULTIPLAYER=1, MODE_ZOMBIES=2, MODE_CAMPAIGN=3},
        LobbyType={LOBBY_TYPE_GAME=1}, UIVisibilityBit={BIT_ROUND_END_KILLCAM=1, BIT_FINAL_KILLCAM=2}}
Engine = {
  SetDvar=function(name,value) safe[name]=value; settingsWrites=settingsWrites+1 end,
  IsInGame=function() return inGame end, IsDemoPlaying=function() return demo end,
  CurrentSessionMode=function() return mode end,
  GetGametypeSetting=function(name) if name=='allowInGameTeamChange' then return allowTeams end; return 0 end,
  IsVisibilityBitSet=function() return killcam end,
  Team=function() return 'TEAM_ALLIES' end, IsLobbyHost=function() return false end,
  GetLobbyUIScreen=function() return 0 end,
  Exec=function() error('The addon must not issue gameplay commands') end,
}
LobbyData={UITargetFromId=function() return {maxClients=18} end}
CoD = {IsTeamChangeAllowed=function() return false end}
ChooseTeam=function() chosenTeams=chosenTeams+1 end
ChooseClass=function() end; QuitGame_MP=function() end; OpenPCQuit=function() end
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
  return {rows=rows,buttonList={setTopBottom=function() end,setVerticalCount=function() end}},'widget-token'
end
CoD.StartMenu_GameOptions={new=newWidget}
LUI={roots={},createMenu={StartMenu=function(...)
  local instance=CoD.StartMenu_GameOptions.new(...)
  return instance,'menu-token'
end}}
function require() end
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


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)
