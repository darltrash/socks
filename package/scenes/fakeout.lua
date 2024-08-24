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

return {
    init = function(self)
        eng.videomode(400, 300)
        eng.far(0, 0x00000000)

        self.t = 0
        self.speed = 0
    end,

    frame = function(self, delta)
        local str = "FATAL ERROR:"
        local w = ui.text_size(str)

        eng.rect(-138, -49 - self.t, w + 16, 16, 0xFF0353FF)
        ui.print(string, -131, -44 - self.t)

        self.speed = math.min(1, self.speed + delta * 0.01)
        self.t = self.t + (delta * self.speed)

        if self.t > 1200 then
            os.exit()
        end
    end
}
