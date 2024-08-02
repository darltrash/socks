local characters = require "characters"
local language = require "language"
local ui = require "ui"
local mat4 = require "lib.mat4"
local fam = require "lib.fam"

local dialog = {}

dialog.text = ""

local w, h = 450, 350

local tick = eng.load_sound("assets/snd_tick.ogg")

dialog.actor = function(actor)
    local s = {}
    for str in actor:gmatch("([^:]+)") do
        table.insert(s, str)
    end

    if actor:sub(1, 1) == ":" then
        table.insert(s, 1, nil)
    end

    local char = s[1]
    if char then
        dialog.character = characters[char]
        dialog.submesh = "neutral"
    end

    local submesh = s[2]
    if submesh then
        dialog.submesh = submesh
    end

    local direction = s[3]
    if direction then
        dialog.char_at_right = direction == "r"
    end

    dialog.switch_up = 1
end

dialog.say = function(text)
    dialog.text = ""
    dialog.syllables = language.syllabify("* " .. text)
    dialog.scissor = 1
    dialog.busy = true

    repeat
        coroutine.yield()
    until not dialog.busy
end

dialog.follow = function(text)
    for _, v in ipairs(language.syllabify("\n* " .. text)) do
        table.insert(dialog.syllables, v)
    end
    dialog.busy = true

    repeat
        coroutine.yield()
    until not dialog.busy
end

dialog.wait = function(time)
    local final = eng.time + time
    repeat
        coroutine.yield()
    until eng.time >= final
end

dialog.tick = function()
    if dialog.busy then
        dialog.scissor = math.min(#dialog.syllables, dialog.scissor + 0.2)

        local c = dialog.character

        if c and c.sfx and dialog._submesh ~= dialog.submesh then
            local s = c.sfx[dialog.submesh]
            if s then
                eng.sound_play(s, { gain = 2 })
            end
        end
        dialog._submesh = dialog.submesh

        local s = math.floor(dialog.scissor)
        if s ~= dialog._scissor then
            dialog.text = dialog.text .. dialog.syllables[s]
            dialog._scissor = s

            local t = tick
            if c then
                t = fam.choice(c.speech)
            end
            eng.sound_play(t, { gain = 0.4, pitch = math.random(80, 120) / 100 })
        end

        if dialog.scissor == #dialog.syllables then
            if eng.input("jump") then
                dialog.busy = false
            end
        end
    end
end

dialog.frame = function(delta)
    local m = 10
    local mx = 70
    local hh = 80
    local rw = w - (m * 2) - (mx) - (w / 6)

    local t = dialog.busy and 0 or 1
    dialog.switch_up = fam.decay(dialog.switch_up or 1, t, 3, delta)
    local e = fam.inv_square(dialog.switch_up) * 32

    local r = fam.to_u8((1 - dialog.switch_up) * 1.1)

    local c1 = { 0xd4, 0xbe, 0xfa, r }
    local c2 = { 0x33, 0x00, 0x33, r }
    ui.rounded_rect((-w / 2) + m + mx, (h / 2) - (hh + m) + e, rw, hh, c1)

    if dialog.character then
        ui.print(dialog.text, (-w / 2) + (m * 2) + mx + 66, (h / 2) - hh + e, c2, rw - 80)

        local y = (h / 2) - (160)

        --eng.rect((-w/2)+(m*2)+mx+66, (h/2)-hh-(m/2), rw-80, hh-(m), 0xFF00FFFF

        local avatar = dialog.character.avatar

        eng.draw {
            mesh = avatar,
            range = avatar.submeshes[dialog.submesh],
            model = mat4.from_transform(
                { (-w / 2) + m, y - m + 4 + (e * 2), -0.1 },
                { 0, 0, math.sin(eng.timer) * 0.01 },
                { 160, 160, 1 }
            ),
            tint = { 255, 255, 255, ((r / 255) ^ 2) * 255 },
            texture = { 0, 0, 1, 1 }
        }
    else
        ui.print(dialog.text, (-w / 2) + (m * 2) + mx, (h / 2) - (hh) + e, c2, rw - 80)
    end
end

return dialog
