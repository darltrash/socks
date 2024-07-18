-- Implements some sort of finite state machine for the player controls

local vec3 = require "lib.vec3"
local fam = require "lib.fam"
local assets = require "assets"
local camera = require "camera"

local inventory = {}

local player_animation = {
    --front = {
    { 16 + 0,  0, 32, 32 },
    { 16 + 32, 0, 32, 32 },
    { 16 + 0,  0, 32, 32 },
    { 16 + 64, 0, 32, 32 },
    --},

    back = {
        { 208, 0, 32, 32 },
        { 240, 0, 32, 32 },
        { 208, 0, 32, 32 },
        { 272, 0, 32, 32 },
    },

    on_air = { 112, 0, 32, 32 },
    hurt = { 144, 0, 32, 32 },
    jumping = { 176, 0, 32, 32 },
}

local collider = {
    offset = { -0.1, -0.5, 0.0 },
    size   = { 0.2, 1, 1.0 }
}

local handle_wasd = function(flip_x)
    local vel = { 0, 0, 0 }

    if eng.input("up") then
        vel[1] = -1
    end

    if eng.input("down") then
        vel[1] = vel[1] + 1
    end

    if eng.input("left") then
        vel[2] = -1
        flip_x = -1
    end

    if eng.input("right") then
        vel[2] = vel[2] + 1
        flip_x = 1
    end

    return vec3.normalize(vel), flip_x
end

local function handle_jump(ent, world)
    if ent.on_floor then
        ent.double_jumped = false
    elseif ent.double_jumped then
        return
    end

    if eng.input("jump") == 1 then
        ent.double_jumped = not ent.on_floor

        eng.sound_play(assets.jump, { gain = 0.6 })

        ent.scale[3] = 0.7
        ent.texture = player_animation.jumping
        ent.velocity[3] = 0.6

        for x = 1, 10 do
            local v = vec3.mul_val(vec3.random(3), math.random(10, 50) / 30)
            v[3] = 0

            table.insert(world.particles, {
                position = ent.position,
                velocity = v,
                life = 2,
                scale = math.random(1, 3) / 32,
                decay_rate = 1
            })
        end
    end
end

local checkpoint
local has_died = false

local switch = {
    flashtime = function(ent, world)
        ent.flash_timer = ent.flash_timer - (1 / 30)
        ent.floats = true

        ent.texture = nil
        if (math.floor(ent.flash_timer * 2) % 2 == 0) then
            ent.texture = player_animation.on_air
        end

        if ent.flash_timer <= 0 then
            ent.floats = false
            ent.state = "on_air"
            return true
        end
    end,

    on_floor = function(ent, world)
        ent.tint[4] = math.min(ent.tint[3] + 40, 255)
        if not ent.on_floor then
            ent.state = "on_air"
            return true -- go off.
        end

        if not checkpoint then
            checkpoint = vec3.add(ent.position, { 0, -1, 1 })
        end

        ent.texture = fam.loop_index(player_animation, ent.animation_timer)

        local e = math.floor(ent.animation_timer)
        if e % 2 == 0 and e ~= ent._anim_timer then
            eng.sound_play(assets.floor_hit)
            ent._anim_timer = e
        end

        local vel = vec3.zero3

        if not (world.transition.ease or world.script) then
            vel, ent.flip_x = handle_wasd(ent.flip_x)

            handle_jump(ent, world)
        end

        ent.animation_timer = ent.animation_timer + 0.15

        local moving = vec3.length(vel) > 0.2
        if moving and not ent._moving then
            ent.animation_timer = 2
        end
        ent._moving = moving

        if not moving then
            ent.animation_timer = 1
        end

        local v = vec3.mul_val(vel, 0.3)

        ent.velocity[1] = fam.lerp(ent.velocity[1], v[1], 1 / 4)
        ent.velocity[2] = fam.lerp(ent.velocity[2], v[2], 1 / 4)
    end,

    on_air = function(ent, world)
        if ent.on_floor then
            ent.state = "on_floor"
            ent.scale[3] = 1
            eng.sound_play(assets.floor_hit)
            return true
        end

        table.insert(world.particles, {
            position = ent.position,
            velocity = vec3.zero3,
            life = 1,
            scale = 1 / 16,
            decay_rate = 1
        })

        -- OUT OF BOUNDS
        if ent.position[3] < -3 then
            ent.delete = true


            eng.sound_play(assets.pop)
            world:add_entity {
                type = "explosion",
                position = ent.position,
                intensity = 1,
                boom_counter = has_died and 1.4 or 3,

                in_case_of_boom = function()
                    world:add_entity {
                        type = "player",
                        position = vec3.copy(checkpoint)
                    }
                    has_died = true
                end
            }
            return
        end

        if ent.velocity[3] < 0 then
            ent.scale[3] = fam.lerp(ent.scale[3], 1.3, 1 / 6)
            ent.texture = player_animation.on_air
        else
            ent.scale[3] = fam.lerp(ent.scale[3], 1, 1 / 6)
        end

        local vel = vec3.zero3
        if not world.transition.ease then
            vel, ent.flip_x = handle_wasd(ent.flip_x)

            handle_jump(ent, world)
        end

        local v = vec3.mul_val(vel, 0.3)

        ent.velocity[1] = fam.lerp(ent.velocity[1], v[1], 1 / 8)
        ent.velocity[2] = fam.lerp(ent.velocity[2], v[2], 1 / 8)
    end,

    hurt = function(ent, world)
        if not ent.hurt_counter then
            ent.hurt_counter = 0.4
        end

        ent.texture = player_animation.hurt

        ent.hurt_counter = ent.hurt_counter - (1 / 30)

        ent.velocity[1] = fam.lerp(ent.velocity[1], 0, 1 / 8)
        ent.velocity[2] = fam.lerp(ent.velocity[2], 0, 1 / 8)

        if ent.hurt_counter <= 0 then
            ent.state = "on_air"
            ent.hurt_counter = nil
            return true
        end
    end,

    walk_in = function(ent, world)
        ent.animation_timer = ent.animation_timer + 0.15
        ent.texture = fam.loop_index(player_animation.back, ent.animation_timer)
        ent.tint[4] = ent.tint[4] - 20

        ent.velocity[1] = fam.lerp(ent.velocity[1], 0, 1 / 3)
        ent.velocity[2] = fam.lerp(ent.velocity[2], 0, 1 / 3)
    end
}

return {
    init = function(ent, world)
        ent.texture = player_animation[1]
        ent.camera_focus = camera.focus(ent.position, 5, 20, true)
        ent.animation_timer = 1

        ent.collider = collider

        ent.sphere_collider = {
            offset = { 0, 0, 1.1 },
            size   = { 1, 1, 1 }
        }

        ent.state = ent.state or "on_air"
        ent.id = "player"
        ent.tint = { 255, 255, 255, 255 }

        ent.listener = true

        --ent.position = vec3.add(ent.position, {1, 1, 4})
    end,

    tick = function(ent, world)
        while assert(switch[ent.state])(ent, world) do end

        ent.camera_focus.target = ent.position

        if ent.delete then
            ent.camera_focus.dead = true
        end
    end,

    set_checkpoint = function(checkpoint_)
        checkpoint = vec3.copy(checkpoint_)
    end,

    holding = nil
}
