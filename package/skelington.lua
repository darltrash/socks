local vec3 = require "lib.vec3"
local fam  = require "lib.fam"
local assets = require "assets"

local calacas_waitin = {
    { 144, 32, 32, 32 },
    { 176, 32, 32, 32 },
    { 208, 32, 32, 32 },
    { 176, 32, 32, 32 },
}

local calacas_runnin = {
    { 144, 64, 32, 32 },
    { 176, 64, 32, 32 },
    { 208, 64, 32, 32 },
    { 176, 64, 32, 32 },
} 

local switch = {
    dormant = function (ent, world)
        ent.velocity[1] = (math.sin(eng.timer) ^ (1/9)) * 0.1
        ent.animation_timer = ent.animation_timer + 1/8
        ent.texture = fam.loop_index(calacas_waitin, ent.animation_timer)
    end
}

return {
    init = function (ent, world)
        ent.texture = calacas_waitin[2]
        ent.animation_timer = 1

        ent.collider = {
            offset = {-0.1, -0.25, 0.0},
            size   = { 0.2,   0.5, 1.0}
        }
        
        ent.state = ent.state or "dormant"
        ent.animation_timer = 1
    end,

    tick = function (ent, world)
        while assert(switch[ent.state])(ent, world) do end
    end,
}