-- Implements some sort of finite state machine for the player controls

local vec3 = require "lib.vec3"
local fam  = require "lib.fam"

local player_animation = {
    { 16+ 0, 0, 32, 32 },
    { 16+32, 0, 32, 32 },
    { 16+ 0, 0, 32, 32 },
    { 16+64, 0, 32, 32 },
}

local handle_wasd = function (flip_x)
    local vel = {0, 0, 0}

    if eng.input("left") then
        vel[2] = -1
        flip_x = -1
    end

    if eng.input("right") then
        vel[2] = vel[2] + 1
        flip_x = 1
    end
    
    if eng.input("up") then
        vel[1] = -1
    end

    if eng.input("down") then
        vel[1] = vel[1] + 1
    end

    return vec3.normalize(vel), flip_x
end

local switch = {
    on_floor = function (ent, world)
        if not ent.on_floor then
            ent.state = "on_air"
            return true -- go off.
        end

        local vel, flip = handle_wasd(ent.flip_x)
        ent.flip_x = flip

        if eng.input("jump") then
            ent.velocity[3] = world.superboots and 0.9 or 0.6
        end

        ent.animation_timer = ent.animation_timer + 0.15
        if vec3.length(vel) < 0.9 then
            ent.animation_timer = 1
        end

        ent.texture = fam.loop_index(player_animation, ent.animation_timer)
        
        local v = vec3.mul_val(vel, 0.3)

        ent.scale[3] = 1

        ent.velocity[1] = fam.lerp(ent.velocity[1], v[1], 1/4)
        ent.velocity[2] = fam.lerp(ent.velocity[2], v[2], 1/4)
    end,

    on_air = function (ent, world)
        if ent.on_floor then
            ent.state = "on_floor"

            return true
        end

        if ent.velocity[3] < 0 then
            ent.scale[3] = fam.lerp(ent.scale[3], 1.3, 1/6)
        end

        ent.texture = { 112, 0, 32, 32 }

        local vel, flip = handle_wasd(ent.flip_x)
        ent.flip_x = flip

        local v = vec3.mul_val(vel, 0.3)

        ent.velocity[1] = fam.lerp(ent.velocity[1], v[1], 1/8)
        ent.velocity[2] = fam.lerp(ent.velocity[2], v[2], 1/8)
    end,

    hurt = function (ent, world)
        if not ent.hurt_counter then
            ent.hurt_counter = 0.4
        end

        ent.texture = { 144, 0, 32, 32 }

        ent.hurt_counter = ent.hurt_counter - (1/30)

        ent.velocity[1] = fam.lerp(ent.velocity[1], 0, 1/8)
        ent.velocity[2] = fam.lerp(ent.velocity[2], 0, 1/8)

        if ent.hurt_counter <= 0 then
            ent.state = "on_air"
            ent.hurt_counter = nil
            return true
        end
    end
}

return {
    init = function (ent, world)
        ent.texture = {
            16, 0, 32, 32
        }

        ent.camera_focus = true
        ent.animation_timer = 1

        ent.collider = {
            offset = {-0.1, -1, 0.0},
            size   = { 0.2,  2, 1.0}
        }
        
        ent.state = "on_air"
    end,

    tick = function (ent, world)
        while assert(switch[ent.state])(ent, world) do end
    end
}