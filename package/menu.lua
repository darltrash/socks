local ui = require "ui"

eng.set_room {
    frame = function ()
        ui.print_center("It works!", -150, -150, 300, 300)
    end
}