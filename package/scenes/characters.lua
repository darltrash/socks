local ui = require "ui"
local characters = require "characters"

local chars = {}
for k in pairs(characters) do
    table.insert(chars, k)
end

chars = ui.crap_menu.new(chars)

return {
    init = function()
        eng.videomode(400, 300)
    end,

    tick = function()
        chars:tick()
    end,

    frame = function()
        eng.ambient(0)
        chars:frame(-200 + 10, -100 + 10)
    end
}
