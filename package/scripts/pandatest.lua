local camera = require "camera"
local dialog = require "dialog"

local say, actor, follow = dialog.say, dialog.actor, dialog.follow
local focus, look_at = camera.focus, camera.look_at

return function(self)
    local p = self.entities.map.player.position
    look_at({ -2, 5, 10 }, p, 5)
    actor "lyu:shock"
    say "DAMN IT'S HIGH UP HERE!"

    look_at({ p[1] + 1, p[2], p[3] + 0 }, { p[1], p[2], p[3] + 1 }, 5)
    actor "lyu:uncomfortable"
    follow "oh wait, no..."
    follow "wow, you look awful down here"

    focus(p, 3, 2)
    say "Is this better...?"

    focus(p, 5, 1)
    actor "lyu:neutral"
    follow "Much better, isn't it?"

    actor "lyu:weedz"
    say "*obligatory weed joke*"
end
