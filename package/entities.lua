local vec3 = require "lib.vec3"
local fam = require "lib.fam"
local assets = require "assets"

local player = require "player"

local init = {
    player = player.init,

    capeman = function(ent, world)
        ent.texture = { 80, 32, 32, 32 }
        ent.floats = true
    end,

    explosion = function(ent, world)
        local intensity = ent.intensity or 5

        for x = 1, intensity * 5 do
            table.insert(world.particles, {
                position = ent.position,
                velocity = vec3.mul_val(vec3.random(3), math.random(10, intensity * 14) / 10),
                life = 2,
                scale = math.random(1, intensity) / 32,
                decay_rate = 1
            })
        end

        ent.boom_counter = ent.boom_counter or 3
    end,

    door = function(ent, world)
        local t = world.teleporters[ent.tokens[2]]
        if t then
            t.link = ent
            ent.link = t
        else
            world.teleporters[ent.tokens[2]] = ent
        end

        ent.interactable = true
        ent.floats = true

        ent.lock = ent.tokens[3]
        ent.mesh = ent.lock and assets.lock or nil
    end,

    jumpybumpy = function(ent)
        ent.floats = true
        ent.no_shadow = true
        ent.mesh = assets.fan
        ent.speed = 0
        ent.particle_timer = 0
    end,

    switch = function(ent, world)
        ent.floats = true
        ent.no_shadow = true
        ent.switch = ent.tokens[2]
        ent.delete = ent.switch == nil
        ent.interactable = true
        ent.mesh = assets.switch
        ent.collider = {
            offset = { -0.5, -0.5, -0.5 },
            size = { 1, 1, 1 }
        }
    end,

    key = function(ent, world)
        ent.floats = true
        ent.texture = { 80, 64, 16, 16 }
        ent.position[3] = ent.position[3] + 1
        ent.particle_timer = 0
    end,

    follower = function(ent, world)
        ent.particle_timer = 0
        ent.floats = true
        ent.tint = { 255, 255, 255, 255 }
    end,

    npc = function(ent, world)
        local ok, r = pcall(require, "scripts." .. ent.tokens[2])
        ent.texture = { 496, 480, 16, 16 }

        if r then
            r(ent)
        end

        ent.interactable = ent.on_interact and true or false
        ent.floats = not ent.collider
    end
}

local tick = {
    player = player.tick,

    capeman = function(ent, world)
        ent.texture[1] = 80 + (math.floor(eng.timer * 2) % 2) * 32

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

    explosion = function(ent, world)
        ent.boom_counter = ent.boom_counter - (1 / 30)

        if ent.boom_counter <= 0 then
            ent.delete = true
            if ent.in_case_of_boom then
                ent.in_case_of_boom()
            end
        end
    end,

    door = function(ent, world)
        local p = world.entities.map.player

        if p and ent.interacting then
            if ent.lock then
                -- check if player has key to door
                local h = player.holding
                if not h then
                    return
                end

                if h.key ~= ent.lock then
                    return
                end

                -- it has key to door so we'll get rid of the lock icon and the lock itself
                ent.lock = false
                ent.mesh = nil
                player.holding = nil
            end

            world.transition.ease = "in"
            p.state = "walk_in"

            world.transition.callback = function()
                --world.camera.instant = true
                p.position = vec3.add(ent.link.position, { 0.3, 0, 0.3 })
                p.ghost_mode = true
                p.state = "on_air"
                player.set_checkpoint(p.position)
            end
        end
    end,

    jumpybumpy = function(ent, world)
        local speed_up = true

        local l = ent.tokens[2]
        if l then
            speed_up = world.switches[l]
        end

        ent.speed = fam.lerp(ent.speed, speed_up and 1 or 0, 1 / 2)

        ent.rotation[3] = eng.timer * ent.speed * 4

        local p = world.entities.map.player
        local render = true
        if p then
            local inside_cylinder =
                (vec3.distance(p.position, ent.position, 2) <= ent.area)
                and (math.abs(p.position[3] - ent.position[3]) <= 4)

            render = vec3.distance(p.position, ent.position) < 15

            if inside_cylinder then
                p.velocity = vec3.add(p.velocity, { 0, 0, ent.speed * 0.12 })
            end
        end

        ent.particle_timer = ent.particle_timer + ent.speed
        if ent.particle_timer > 3 and render then
            local n = vec3.random(3)
            n[3] = 0
            local t = vec3.add(ent.position, n)

            table.insert(world.particles, {
                position = t,
                velocity = { 0, 0, 4 },
                life = ent.speed,
                scale = 2 / 16,
                decay_rate = 0.2
            })

            ent.particle_timer = 0
        end
    end,

    switch = function(ent, world)
        if ent.interacting then
            eng.sound_play(assets.switch_click)
            world.switches[ent.switch] = not world.switches[ent.switch]

            ent.scale[3] = -ent.scale[3]
        end
        ent.ghost_mode = true
    end,

    key = function(ent, world)
        local render = true

        --if world.camera.lerped then
        --    render = vec3.distance(world.camera.lerped, ent.position) < 15
        --end

        local p = world.entities.map.player
        if p and vec3.distance(p.position, ent.position) < 3 then
            ent.delete = true
            eng.sound_play(assets.grab, { gain = 0.3 })
            player.holding = world:add_entity {
                type = "follower",
                position = ent.position,
                texture = ent.texture,
                key = ent.tokens[2]
            }
            return
        end

        ent.particle_timer = ent.particle_timer + 0.5
        if ent.particle_timer > 3 and render then
            local n = vec3.random(3)
            local t = vec3.add(ent.position, n)

            table.insert(world.particles, {
                position = t,
                velocity = { 0, 0, 0.1 },
                life = 1,
                scale = 2 / 16,
                decay_rate = 0.2,
                light = math.random(1, 3) == 3
                    and { 0.3, 0.05, 0.1 }
                    or false,
                color = { 245, 173, 49, 255 }
            })

            ent.particle_timer = 0
        end

        ent.velocity[3] = math.sin(eng.timer * 4) * 0.1
    end,

    follower = function(ent, world)
        local p = world.entities.map.player

        if ent.disable then
            ent.tint[4] = math.max(0, ent.tint[4] - 20)
            if ent.tint[4] == 0 then
                ent.delete = true
            end
            return
        end

        ent.alpha = fam.lerp(ent.alpha or 0, p and 1 or 0, 1 / 3)
        ent.tint[4] = fam.to_u8(ent.alpha)

        if p then
            ent.flip_x = p.flip_x or 0
            local t = vec3.add(p.position, { 1 / 16, p.flip_x or 0, 0 })
            ent.velocity = vec3.mul_val(vec3.sub(t, ent.position), 0.3)

            if player.holding ~= ent then
                ent.disable = true
            end
        end

        local render = true

        if world.camera.lerped then
            render = vec3.distance(world.camera.lerped, ent.position) < 15
        end

        ent.particle_timer = ent.particle_timer + 0.5
        if ent.particle_timer > 3 and render then
            local n = vec3.random(3)
            local t = vec3.add(ent.position, n)

            table.insert(world.particles, {
                position = t,
                velocity = { 0, 0, 0.1 },
                life = 1,
                scale = 2 / 16,
                decay_rate = 0.2,
                light = math.random(1, 3) == 3
                    and { 0.3, 0.05, 0.1 }
                    or false,
                color = { 245, 173, 49, 255 }
            })

            ent.particle_timer = 0
        end
    end,

    lightflicker = function(ent, world)
        ent.cycles = (ent.cycles or 0) + 1

        if ent.cycles == 2 then
            ent.light.off = math.random(1, 3) == 1
            ent.cycles = 0
        end
    end,

    npc = function(ent, world)
        if ent.interacting and ent.on_interact then
            world:routine(ent.on_interact)
        end
    end
}

return {
    init = init,
    tick = tick,
}
