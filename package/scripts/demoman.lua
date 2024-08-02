local fam = require "lib.fam"
local vec = require "lib.vec3"

local function hslToRgb(h, s, l)
    if s == 0 then return l, l, l end
    local function to(p, q, t)
        if t < 0 then t = t + 1 end
        if t > 1 then t = t - 1 end
        if t < .16667 then return p + (q - p) * 6 * t end
        if t < .5 then return q end
        if t < .66667 then return p + (q - p) * (.66667 - t) * 6 end
        return p
    end
    local q = l < .5 and l * (1 + s) or l + s - l * s
    local p = 2 * l - q
    return to(p, q, h + .33334), to(p, q, h), to(p, q, h - .33334)
end

return function(ent)
    ent.texture = { 464, 480, 32, 32 }
    ent.tint = { 255, 255, 255, 255 }

    ent.on_interact = function(s)
        s.actor "demo:happy"
        s.say "Hey! Hello world!"
        s.follow "Check this out! a new ability i've been developing"
        s.actor "demo:loading"
        s.say "LOADING 'scripts/COOLASS_SCENE.lua'"

        local l = s.entities.map.thelightabove

        s.look_at(
            { l.position[1] + 4, l.position[2], l.position[3] - 1 },
            { l.position[1], l.position[2], l.position[3] - 1 },
            2
        )

        s.wait(2)

        assert(l)

        l.light.color = { 0, 3, 0 }

        s.sfx "snd_switch"

        s.wait(1)

        ent.on_interact = function()
            s.actor "demo:happy"
            s.say "Cool right?"
            s.follow "wait, ima do it again"

            s.look_at(
                { l.position[1] + 4, l.position[2], l.position[3] - 1 },
                { l.position[1], l.position[2], l.position[3] - 1 },
                2
            )
            for x = 1, 100 do -- 120
                l.light.color = { x / 9, x / 2, x / 9 }
                coroutine.yield()
            end
            l.light.color = { 0, 0, 0 }

            s.sfx "snd_switch"

            s.wait(1)

            s.actor "demo:neutral"
            s.say "... it broke"

            ent.on_interact = function()
                s.say "life sucks sometimes"

                s.follow "Welp, I'm gone!"

                s.sfx("snd_mistery", true)
                for x = 1, 15 do
                    ent.tint[4] = ent.tint[4] - 17
                    coroutine.yield()
                end
                ent.delete = true
            end
        end
    end
end
