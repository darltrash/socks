local vec3 = require "lib.vec3"
local fam  = require "lib.fam"

local player = require "player"

local init = {
    player = player.init,

    capeman = function (ent, world)
        ent.texture = { 80, 32, 32, 32 }
        ent.floats = true
    end
}

local tick = {
    player = player.tick,

    capeman = function (ent, world)
        ent.texture[1] = 80 + (math.floor(eng.timer*2) % 2) * 32

        if world.kill_em_guys then
            ent.delete = true
        end

        local p = world.entities.map.player
        if p then
            local d = vec3.sub(p.position, ent.position)

            local n = vec3.normalize(d)
            ent.velocity = vec3.mul_val(n, 0.1)

            if vec3.length(d) < 0.5 then
                p.velocity = vec3.add(p.velocity, { n[1], n[2], 0.3 })
                p.state = "hurt"
            end

            local t = fam.sign(d[2])
            if t ~= 0 then
                ent.flip_x = t
            end
        end
    end
}

return {
    init = init,
    tick = tick
}