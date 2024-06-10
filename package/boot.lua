-- boot
local noop = function (...) end

-- our fricked up, nicer version of print.
local ogprint = print

print = function (...)
   ogprint("env:", ...)
end

print("i'm here now.")

-- custom require because we will only read 
-- files embedded into the binary itself
-- TODO: FIX THIS
local require_cache = {}
require = function (str)
    if not require_cache[str] then
        local f1 = string.gsub(str, "%.", "/") .. ".lua"
        local t = eng.read(f1)
        
        if not t then
            local f2 = string.gsub(str, "%.", "/") .. "/init.lua"
            t = eng.read(f2)

            assert(t, "File '"..f1.."', '"..f2.."' doesn't exist!")
        end

        local g = load(t, str..".lua")()
        require_cache[str] = g
    end

    return require_cache[str]
end

-- cool crash screen, this essentially assumes
-- nothing in the C side is unrecoverable.
local function on_error(err)
    local trace = debug.traceback("", 2)

    trace = trace:gsub("\t", "→ "):gsub("stack traceback:", "TRACEBACK:")

    local ui = require "ui"

    eng.music_stop()

    eng.videomode(400, 300)

    eng.tick = noop
    eng.frame = function ()
        eng.far(0, 0x00000000)
        local str = "FATAL ERROR:"
        local w = ui.text_size(str)

        eng.rect(-138, -49, w+16, 16, 0xFF0353FF)
        ui.print(str.."\n\n"..err.."\n"..trace.."\n", -131, -44)
    end

    if eng.os == "windows" then
        ogprint("\nENV HALT:\n", err.."\n"..trace.."\n")
    else
        ogprint("\n\27[31mENV HALT:\n", err.."\n"..trace.."\n\27[0m")
    end
end


-- gamemaker-esque rooms, not scenes, it's different.
local room

eng.set_room = function (new_room, ...)
    print("wazzaaa! room change.")
    assert(type(new_room)=="table")

    new_room.init  = new_room.init  or noop
    new_room.tick  = new_room.tick  or noop
    new_room.frame = new_room.frame or noop

    room = new_room
    xpcall(room.init, on_error, room, ...)

    return room
end

-- actual callback bs, all protected
eng.tick = function (timestep)
    if not room.tick then return end

    xpcall(room.tick, on_error, room, timestep)
end


eng.frame = function (alpha, delta, focused)
    -- i deeply apologize for rendering frames even when the
    -- game is not in focus.

    if focused then
        eng.timer = eng.timer + delta
    end

    eng.focused = focused

    if not room.frame then return end

    xpcall(room.frame, on_error, room, alpha, delta)
end

eng.timer = 0

if os.getenv("BSKT_LEVEL_VIEW") then
    xpcall(require, on_error, "viewer")
    return
end

-- try to load the game 
xpcall(require, on_error, "game")