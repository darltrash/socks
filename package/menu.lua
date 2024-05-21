local mat4 = require "lib.mat4"
local vec3 = require "lib.vec3"

local ui = require "ui"
local assets = require "assets"

local state = {
    camera = {
        distance = 3,
        target = {0, 0, 0.5},

        -- lerped = {0, 0, 0}
    },

    init = function (state)
        eng.ambient(0xFFFFFFFF)
        eng.far(10000, 0x3c1391ff)
        eng.dithering(false)
    end,
    
    frame = function (state, alpha, delta)
        local ox = math.floor((eng.timer * 4) % 2) * 32

        eng.render {
            mesh = assets.plane,
            model = mat4.identity(),
            texture = {
                16+ox, 0, 32, 32
            }
        }

        eng.render {
            mesh = assets.plane,
            model = mat4.from_translation {-1, 0, 0},
            texture = {
                0, 32, 32, 32
            }
        }

        do
            local cam = state.camera

            local eye = vec3.add (
                cam.target,

                vec3.mul_val (
                    {1, 0, 0.5},
                    cam.distance
                )
            )

            cam.lerped = vec3.lerp(cam.lerped or eye, eye, delta)

            eng.camera(cam.lerped, cam.target)
        end
    end
}

eng.set_room(state)