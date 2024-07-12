local dialog = require "dialog"

local say, actor, follow = dialog.say, dialog.actor, dialog.follow

return function ()
    while true do
        actor "lyu:shock"
        say "NO WAY! OH WOW! NO WAY! FOR REAL? NO WAY! OH MY GOD! NO WAYYY!"

        actor "lyu:uncomfortable"
        follow "oh wait, no..."
    end
end