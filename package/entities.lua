local vec3 = require "lib.vec3"
local fam  = require "lib.fam"
local assets = require "assets"

local player = require "player"

local init = {
    player = player.init,

    capeman = function (ent, world)
        ent.texture = { 80, 32, 32, 32 }
        ent.floats = true
    end,

    explosion = function (ent, world)
        local intensity = ent.intensity or 5

        for x=1, 20 do
            table.insert(world.particles, {
                position = ent.position,
                velocity = vec3.mul_val(vec3.random(3), math.random(10, intensity*14)/10),
                life = 2,
                scale = math.random(1, intensity)/32,
                decay_rate = 1
            })
        end
        
        ent.boom_counter = 3
    end,

    door = function (ent, world)
        local t = world.teleporters[ent.tokens[2]]
        if t then
            t.link = ent
            ent.link = t
        else
            world.teleporters[ent.tokens[2]] = ent
        end

        ent.interactable = true
        ent.floats = true
    end,

    jumpybumpy = function (ent)
        ent.floats = true
        ent.no_shadow = true
        ent.mesh = assets.fan
        ent.speed = 0
    end,

    switch = function (ent, world)
        ent.floats = true
        ent.no_shadow = true
        ent.switch = ent.tokens[2]
        ent.delete = ent.switch == nil
        ent.interactable = true
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
    end,

    explosion = function (ent, world)
        ent.boom_counter = ent.boom_counter - (1/30)

        if ent.boom_counter <= 0 then
            ent.delete = true
            if ent.in_case_of_boom then
                ent.in_case_of_boom()
            end
        end
    end,

    door = function (ent, world)
        local p = world.entities.map.player
        if p and ent.interacting then
            world.entities.map.player = nil

            p.delete = true

            world:add_entity {
                type = "player",
                position = vec3.add(ent.link.position, {0.2, 0, 0.2})
            }
        end
    end,

    jumpybumpy = function (ent, world)
        local speed_up = true

        local l = ent.tokens[2]
        if l then
            speed_up = world.switches[l]
        end

        ent.speed = fam.lerp(ent.speed, speed_up and 1 or 0, 1/2)

        ent.rotation[3] = eng.timer * ent.speed * 4

        do
            local n = vec3.random(3)
            n[3] = 0
            local t = vec3.add(ent.position, n)
    
            table.insert(world.particles, {
                position = t,
                velocity = {0, 0, 4},
                life = ent.speed,
                scale = 2/16,
                decay_rate = 0.4
            })
        end
    end,

    switch = function (ent, world)
        if ent.interacting then
            world.switches[ent.switch] = not world.switches[ent.switch]
        end
    end
}

return {
    init = init,
    tick = tick
}