---@diagnostic disable duplicate-set-field

-- our fricked up, nicer version of print.
local ogprint = print

print = function(...)
    ogprint("env:", ...)
end

print("i'm here now.")

-- cool crash screen, this essentially assumes
-- nothing in the C side is unrecoverable.
local on_error = function(err)
    local trace = debug.traceback("", 2)

    trace = trace:gsub("\t", "→ "):gsub("stack traceback:", "TRACEBACK:")

    local ui = require "ui"

    --eng.music_stop()

    eng.videomode(400, 300)

    eng.set_room {
        frame = function()
            eng.far(0, 0x00000000)
            local str = "FATAL ERROR:"
            local w = ui.text_size(str)

            eng.rect(-138, -49, w + 16, 16, 0xFF0353FF)
            ui.print(str .. "  - PLEASE REPORT THIS! \n\n" .. err .. "\n" .. trace .. "\n", -131, -44, 0xFFFFFFFF)
        end
    }

    if eng.os == "windows" or os.getenv("no_color") then
        ogprint("\nENV HALT:\n", err .. "\n" .. trace .. "\n")
    else
        ogprint("\n\27[31mENV HALT:\n", err .. "\n" .. trace .. "\n\27[0m")
    end
end


-- gamemaker-esque rooms, not scenes, it's different.
local room

eng.set_room = function(new_room, ...)
    print("wazzaaa! room change.")
    assert(type(new_room) == "table")

    room = new_room

    if new_room.init then
        xpcall(new_room.init, on_error, room, ...)
    end

    return room
end

-- actual callback bs, all protected
eng.tick = function(timestep)
    if not room.tick then return end

    xpcall(room.tick, on_error, room, timestep)
end


eng.frame = function(alpha, delta, focused)
    -- i deeply apologize for rendering frames even when the
    -- game is not in focus.

    if not focused then
        delta = 0
    end

    eng.timer = eng.timer + delta
    eng.focused = focused

    if not room.frame then return end

    xpcall(room.frame, on_error, room, alpha, delta)
end

eng.timer = 0

local function load(what, ...)
    local _, game = xpcall(function(params, ...)
        local a = require(params)
        eng.set_room(a, ...)
    end, on_error, what, ...)
end

load("game")
--load("tester")
