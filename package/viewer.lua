local mat4 = require "lib.mat4"
local vec3 = require "lib.vec3"
local json = require "lib.json"

local focused = false
local target = {0, 0, 0}

local world_mesh
local lights

eng.ambient(0x19023dff)
eng.far(60, 0x0c031aff)
eng.videomode(500, 400)

local plane = eng.load_model("assets/mod_plane.exm")

eng.set_room {
    tick = function (self, timestep)
        if eng.focused ~= focused then
            focused = eng.focused

            eng.reload_tex()

            local level = os.getenv("BSKT_LEVEL_VIEW")
            world_mesh = eng.load_model(level)

            local data = json.decode(world_mesh.extra)

            lights = {}
            for _, light in ipairs(data.lights) do
                table.insert(lights, {
                    position = light.position,
                    color = vec3.mul_val(light.color, light.power * math.exp(-4.2))
                })
            end
        end

        local vel = 0.5

        if eng.input("up") then
            target[1] = target[1] - vel
        end
    
        if eng.input("down") then
            target[1] = target[1] + vel
        end
    
        if eng.input("left") then
            target[2] = target[2] - vel
        end
    
        if eng.input("right") then
            target[2] = target[2] + vel
        end

        if eng.input("jump") then
            target[3] = target[3] + vel
        end

        if eng.input("attack") then
            target[3] = target[3] - vel
        end
    end,

    frame = function ()
        do
            local eye = vec3.add (
                target,

                vec3.mul_val (
                    {1, 0, 0.5},
                    5
                )
            )

            eng.camera(eye, target, {0, 0, 1})
        end

        for _, light in ipairs(lights) do
            eng.light(light.position, light.color)
        end

        eng.render {
            mesh = plane,
            texture = {16, 0, 32, 32},
            model = mat4.from_transform(target, 0, 2)
        }

        eng.render {
            mesh = world_mesh
        }
    end
}