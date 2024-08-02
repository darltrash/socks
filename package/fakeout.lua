eng.set_room {
    frame = function(self)

    end
}

local string =
[[FATAL ERROR:  - DO NOT REPORT THIS!

ENV HALT:
	game.lua:36: Buffer overflow!

TRACEBACK:
→ [C]: in function 'error'
→ game.lua:36: in function 'game.init'
→ [C]: in function 'xpcall'
→ boot.lua:54: in field 'set_room'
→ boot.lua:89: in function <boot.lua:87>
→ [C]: in function 'xpcall'
→ boot.lua:87: in local 'load'
→ boot.lua:93: in main chunk





SLEEPYHEAD:
→ written by Neil

THANKS TO:
→ shakesoda for tinyfx
→ rxi for json.lua, vec.c
→ nothings for stb
→ kuba-- for zip.c
→ PUC from brazil for lua
→ icculus for mojoal.c
→ the SDL team for SDL

SPECIAL THANKS:
→ God.
→ My dad.
→ M.M, S.M, C.K, L.M
→ My fediverse followers.

→ You!





Thanks for playing my game!
Love yourself.
]]


local ui = require "ui"

eng.videomode(400, 300)

eng.set_room {
    init = function()
        eng.far(0, 0x00000000)
    end,

    frame = function()
        local str = "FATAL ERROR:"
        local w = ui.text_size(str)

        eng.rect(-138, -49, w + 16, 16, 0xFF0353FF)
        ui.print(string)
    end
}
