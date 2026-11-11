-- Lists every specialist's bodies and helmets (index, name key, English name, icon) into mvm_dump_*
-- dvars. Run in the game with: lua_exec <path to this file>, then collect with readdump.py.
local lines = {}
local ok, err = pcall(function()
	local mode = Enum.eModes.MODE_MULTIPLAYER
	for _, hero in ipairs(Engine.GetHeroList(mode)) do
		lines[#lines + 1] = table.concat({ "H", hero.bodyIndex, hero.assetName, hero.displayName, Engine.Localize(hero.displayName),
			tostring(hero.disabled) }, "|")
		local custom = Engine.GetHeroCustomizationTable(mode, hero.bodyIndex)
		for index, item in ipairs(custom.bodies) do
			lines[#lines + 1] = table.concat({ "B", hero.bodyIndex, index - 1, item.name, Engine.Localize(item.name), item.icon }, "|")
		end
		for index, item in ipairs(custom.helmets) do
			lines[#lines + 1] = table.concat({ "M", hero.bodyIndex, index - 1, item.name, Engine.Localize(item.name), item.icon }, "|")
		end
	end
end)
if not ok then lines[#lines + 1] = "ERROR " .. tostring(err) end
local text = table.concat(lines, "\n")
local n = 0
for i = 1, #text, 700 do
	n = n + 1
	Engine.SetDvar("mvm_dump_" .. n, text:sub(i, i + 699))
end
Engine.SetDvar("mvm_dump_count", n)
