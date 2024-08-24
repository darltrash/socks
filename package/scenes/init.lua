local assets = require "assets"
local ui = require "ui"

local options = ui.crap_menu {
    "game", "welcome", "save", "tester", "viewer", "fakeout"
}

assets.FUCKITWEBALL = true

return {
    init = function(self)
        eng.videomode(256, 224)
    end,

    tick = function()
        local sel = options:tick()
        if sel then
            eng.set_room(
                require("scenes." .. options[sel])
            )
        end
    end,

    frame = function(self)
        options:frame(10, 10)
    end
}
