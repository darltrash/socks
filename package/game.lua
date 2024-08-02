local mat4 = require "lib.mat4"
local vec3 = require "lib.vec3"
local bump = require "lib.bump"
local json = require "lib.json"
local fam = require "lib.fam"
local bvh = require "bvh"
local dialog = require "dialog"

local blam = require "lib.blam"

local ui = require "ui"
local assets = require "assets"
local entities = require "entities"
local materials = require "material"

local camera = require "camera"

local ball = {
    position = { 0, 0, 0 },
    velocity = { 0, 0, 0 }
}

local w, h = 450, 350

local state = {
    init = function(self, world)
        world = world or "hometown"
        local world_file = "lvl_" .. world

        eng.snapping(1)
        eng.ambient(0x19023dff)
        eng.videomode(w, h)

        if world == "keep" then
            return
        end

        if not assets.loaded(world_file) then
            assets.load_screen({
                world_file,
                "mod_plane",
                "mod_dome",
                "mod_shadow",
                "mod_sphere"
            }, function()
                eng.set_room(self, world)
            end)

            return
        end

        --eng.dithering(false)

        self.far = 150

        self.colliders = bump.newWorld()

        self.entities = {
            map = {},
            buffer = {},
            counter = 0
        }

        self.transition = {
            ease = "out",
            a = 1
        }

        self.lights = {}

        self.easter_time = 5
        self.easter_bunny = ""
        self.easter_eggs = {}

        self.instructions = {}

        self.world_mesh = assets[world_file]
        self.world_mesh.submeshes = nil

        self.particles = {}

        local vertex_format = "<fff"
        local triangles = {}

        for i = 1, #self.world_mesh.data, 24 do
            local p1x, p1y, p1z = string.unpack(vertex_format, self.world_mesh.data, i)
            table.insert(triangles, p1x)
            table.insert(triangles, p1y)
            table.insert(triangles, p1z)
        end

        self.triangles = bvh.new(triangles)

        eng.sound_play(assets.amb_city, { looping = true })

        eng.orientation({ -1, 0, 0 }, { 0, 0, 1 })

        local data = json.decode(self.world_mesh.extra)

        for _, object in ipairs(data.objects) do
            local ent = {}

            ent.position = object.position

            ent.id = object.name:gsub("(%.%d+)$", "")
            ent.tokens = {}
            for word in string.gmatch(ent.id, '([^/]+)') do
                table.insert(ent.tokens, word)
            end
            ent.type = ent.tokens[1] or ent.id

            self:add_entity(ent)
        end

        for _, area in ipairs(data.trigger_areas) do
            area.id = area.name:gsub("(%.%d+)$", "")

            area.tokens = {}
            for word in string.gmatch(area.id, '([^/]+)') do
                table.insert(area.tokens, word)
            end

            area.type = area.tokens[1] or area.id
            area.area = area.size and (vec3.length(area.size) / 2)

            local t = area.tokens[2] or ""

            local p = vec3.sub(area.position, area.size)
            local s = vec3.mul_val(area.size, 2)

            if area.type == "solid" then
                area.top = p[3] + s[3]
                self.colliders:add(area, p[1], p[2], p[3], s[1], s[2], s[3])

                --        elseif area.type=="area" and area_types[t] then
                --            table.insert(self.areas, {
                --                type = area_types[t],
                --                t = t,
                --                p[1], p[2], p[3], s[1], s[2], s[3]
                --            })
            elseif area.type == "speaker" then
                if area.tokens[2] then
                    local src = assets[area.tokens[2]]
                    local snd = eng.load_sound(src, true)

                    eng.sound_play(snd, {
                        position = area.position,
                        looping = true,
                        gain = area.area * 0.5,
                    })
                end
            else
                self:add_entity(area)
            end
        end

        for _, light in ipairs(data.lights) do
            light.id = light.name:gsub("(%.%d+)$", "")

            light.tokens = {}
            for word in string.gmatch(light.id, '([^/]+)') do
                table.insert(light.tokens, word)
            end

            light.position = light.position
            light.type = light.tokens[1] or light.id

            light.floats = true

            for word in string.gmatch(light.id, '([^/]+)') do
                table.insert(light.tokens, word)
            end

            light.light = {
                offset = { 0, 0, 0 },
                color = vec3.mul_val(light.color, light.power * math.exp(-4.2))
            }

            self:add_entity(light)
        end
    end,

    add_entity = function(self, ent)
        if not ent.id then
            ent.id = self.entities.counter
            self.entities.counter = self.entities.counter + 1
        end

        local a = entities.init[ent.type]
        if a then
            a(ent, self)
        end

        ent.position = ent.position or { 0, 0, 0 }
        ent.velocity = ent.velocity or { 0, 0, 0 }
        ent.scale = ent.scale or { 1, 1, 1 }
        ent.rotation = ent.rotation or { 0, 0, 0 }

        ent._position = vec3.copy(ent.position)
        ent._scale = vec3.copy(ent.scale)
        ent._rotation = vec3.copy(ent.rotation)

        table.insert(self.entities.buffer, ent)
        self.entities.map[ent.id] = ent

        if ent.collider then
            local p = vec3.add(ent.position, ent.collider.offset)
            local s = ent.collider.size
            self.colliders:add(ent, p[1], p[2], p[3], s[1], s[2], s[3])
        end

        return ent
    end,

    routine = function(self, func)
        if self.script then
            coroutine.close(self.script)
        end

        self.script = coroutine.create(func)
    end,

    tick = function(self, timestep)
        --if eng.text() == "e" then
        --    print("e")
        --    self:routine(require("scripts.pandatest"))
        --end

        assets.tick()

        do
            local t = eng.text()
            if #t > 0 then
                self.easter_bunny = self.easter_bunny .. t:lower()
                self.easter_time = 5
            end

            self.easter_time = self.easter_time - (1 / 30)
            if self.easter_time < 0 then
                self.easter_bunny = ""
            end

            local rabbit_barbecue = true
            local l = #self.easter_bunny
            for name, easter in pairs(self.easter_eggs) do
                if name:sub(1, l) == self.easter_bunny then
                    rabbit_barbecue = false

                    if l == #name then
                        self.easter_bunny = ""
                        easter(self)
                        break
                    end
                end
            end

            if rabbit_barbecue then
                self.easter_bunny = ""
            end
        end

        self.interactable = false

        local query_bvh = function(triangles, min, max)
            local t = self.triangles:intersectAABB({ min = min, max = max })
            for k, v in ipairs(t) do
                for x = 1, 3 do
                    table.insert(triangles, v[x])
                end
            end
        end

        local p = self.entities.map.player

        -- https://www.youtube.com/watch?v=-j7pu9RlOUY

        --        if eng.input("menu")==1 then
        --            ball.position = vec3.add(p.position, {0, 0, 1})
        --            ball.velocity = {0, 0, 0.0}
        --        end
        --
        --        do
        --            ball.velocity = vec3.add(ball.velocity, { 0, 0, -0.05 })
        --
        --            ball.position, ball.velocity = blam.response_update(ball.position, ball.velocity, {0.1, 0.1, 0.1}, query_bvh)
        --        end
        --
        --        for _, ball in ipairs(balls) do
        --            local _, planes
        --            ball.position, _, planes = slam.check(ball.position, ball.velocity, 0.5, f)
        --
        --            print(table.unpack(ball.position))
        --
        --            ball.velocity = vec3.add(ball.velocity, { 0, 0, -0.05 })
        --
        --            for _, c in ipairs(planes) do
        --                ball.velocity = vec3.mul_val(c.normal, vec3.length(ball.velocity) * 0.8)
        --            end
        --        end

        dialog.tick()

        for i, ent in ipairs(self.entities) do
            local a = entities.tick[ent.type]
            if a then
                a(ent, self)
            end

            ent._position = vec3.copy(ent.position)
            ent._scale = vec3.copy(ent.scale)
            ent._rotation = ent.rotation

            -- Handle some animations
            if
                ent.texture and
                ent._texture and
                not fam.array_compare(ent._texture, ent.texture) then
                ent.texture_swap = 0.85
            end

            ent._texture = ent.texture

            if ent.flip_x then
                ent.scale[2] = fam.lerp(ent.scale[2], ent.flip_x, 1 / 3)
            end


            -- Simulate gravity
            if (not ent.floats) and ent.velocity[3] > -4 then
                ent.velocity[3] = ent.velocity[3] - 0.05
            end


            -- Handle collision
            if self.colliders:hasItem(ent) then
                local new_position = vec3.add(ent.position, ent.velocity)
                local hs = ent.collider.offset
                local t = vec3.add(new_position, hs)

                if ent.ghost_mode then
                    self.colliders:update(ent, t[1], t[2], t[3])
                    self.velocity = { 0, 0, 0 }
                    ent.ghost_mode = false
                else
                    local k = { self.colliders:move(ent, t[1], t[2], t[3]) }

                    local collisions = k[4]
                    ent.on_floor = false
                    for _, col in ipairs(collisions) do
                        if col.normal.z > 0.5 then
                            ent.on_floor = true
                        end
                    end

                    ent.velocity = vec3.sub(vec3.sub(k, hs, 3), ent.position)
                end
            end

            local coll = ent.sphere_collider
            if false and coll then
                local p = vec3.add(ent.position, coll.offset)
                local _, velocity, planes = blam.response_update(p, ent.velocity, coll.size, query_bvh, 1)

                ent.on_floor = false

                for _, plane in ipairs(planes) do
                    if vec3.dot(plane.normal, { 0, 0, 1 }) > 0.8 then
                        ent.on_floor = true

                        break
                    end
                end

                ent.velocity = velocity
            end

            ent.position = vec3.add(ent.position, ent.velocity)

            ent.interacting = false
            if p and ent.interactable and not (self.transition.ease or self.script) then
                local player_on_area = vec3.distance(p.position, ent.position) < (ent.area or 2)

                if player_on_area then
                    self.interactable = true
                    ent.interacting = eng.input("menu") == 1
                end
            end

            -- performs a swap remove if an entity has the .delete property
            if ent.delete then
                table.remove(self.entities, i)

                if self.colliders:hasItem(ent) then
                    self.colliders:remove(ent)
                end
            end
        end

        for _, ent in ipairs(self.entities.buffer) do
            table.insert(self.entities, ent)
        end

        self.entities.buffer = {}

        self.kill_em_guys = false

        if not self.transition.ease then
            if self.script then
                ---@class ScriptEnv
                ---@field self table
                ---@field entities table
                ---@field say fun(text: string)
                ---@field actor fun(actor: string)
                ---@field follow fun(follow: string)
                ---@field focus fun(target: number[], dist: number, speed: number, is_listener: boolean?)
                ---@field look_at fun(eye: number[], target: number[], speed: number, is_listener: boolean?)
                ---@field force_focus fun(target: number[], dist: number, is_listener: boolean?)

                local scripting = {
                    self = self,
                    entities = self.entities,

                    say = dialog.say,
                    ask = dialog.ask,
                    actor = dialog.actor,
                    follow = dialog.follow,

                    focus = camera.focus,
                    look_at = camera.look_at,
                    force_focus = camera.force_focus,

                    sfx = function(music, dont_wait)
                        local inst = {
                            type = "sfx",
                            src = music
                        }

                        table.insert(self.instructions, inst)

                        if not dont_wait then
                            repeat coroutine.yield() until inst.done
                        end

                        return inst
                    end,

                    wait = function(seconds)
                        local target = eng.timer + seconds
                        while eng.timer < target do
                            coroutine.yield()
                        end
                    end,

                    image = function(what)
                        local inst = {
                            type = "image",
                            src = what
                        }

                        table.insert(self.instructions, inst)

                        repeat coroutine.yield() until inst.done
                    end
                }

                if coroutine.status(self.script) == "dead" then
                    self.script = nil
                    print("SCRIPT FINISHED :)")
                    return
                end

                local ok, err = coroutine.resume(self.script, scripting)
                if not ok then
                    self.script = nil
                    print("SCRIPT ERROR: " .. err)
                end
            end

            for i, inst in ipairs(self.instructions) do
                if inst.type == "image" then
                    inst.done = true
                    self.transition.ease = "in"
                    self.transition.a = 0
                    self.transition.callback = function()
                        local screen = require "screen"
                        eng.set_room(screen, inst.src, function()
                            eng.set_room(self, "keep")
                        end)
                    end
                    table.remove(self.instructions, i)
                elseif inst.type == "sfx" then
                    if not inst.real_source then
                        inst.real_source = assets[inst.src]
                        inst.audio = eng.sound_play(inst.real_source)
                    end

                    if eng.sound_state(inst.audio) ~= "playing" then
                        inst.done = true
                        table.remove(self.instructions, i)
                    end
                end
            end
        end
    end,

    frame = function(self, alpha, delta)
        eng.far(self.far, 0x0c031aff)

        local cam = self.camera

        local dome = assets.mod_dome

        for _, ent in ipairs(self.entities) do
            if ent.invisible then goto continue end

            if ent.light and not ent.light.off then
                eng.light(ent.position, ent.light.color)
            end

            local visible = (ent.texture or ent.mesh)
            if not visible then goto continue end

            local p = vec3.lerp(ent._position, ent.position, alpha)
            local s = vec3.lerp(ent._scale, ent.scale, alpha)
            local r = {
                fam.angle_lerp(ent._rotation[1], ent.rotation[1], alpha),
                fam.angle_lerp(ent._rotation[2], ent.rotation[2], alpha),
                fam.angle_lerp(ent._rotation[3], ent.rotation[3], alpha),
            }

            ent.texture_swap = fam.decay(ent.texture_swap or 1, 1, 0.6, delta)

            local real_scale = s

            if not ent.mesh then
                real_scale = vec3.mul(
                    s,
                    {
                        0,
                        ent.texture[3] / 16,
                        (ent.texture[4] / 16) * (ent.texture_swap or 1),
                    }
                )
            end

            local model = mat4.from_transform(p, r, real_scale)

            -- assume mat4.look_at(eye, towards, up) exists


            eng.render {
                model = model,
                mesh = ent.mesh or assets.mod_plane,
                tint = ent.tint,
                texture = ent.texture
            }


            -- Calculate and render shadow
            local has_collider_on_air = ent.collider and not ent.on_floor


            --[[            if (has_collider_on_air or ent.floats) and not ent.no_shadow then
                local MAX_SHADOW_DEPTH = 16
                local RAY_THICKNESS = 0.06
                local RAY_BIAS = 0.1

                local pos = vec3.copy(p)
                pos[3] = -MAX_SHADOW_DEPTH

                -- Bump has no raycasting function so I simply check against a veeery
                -- thin little parallelepiped
                local items, len = self.colliders:queryCube (
                    p[1]-(RAY_THICKNESS/2), p[2]-(RAY_THICKNESS/2), p[3]-MAX_SHADOW_DEPTH,
                    RAY_THICKNESS, RAY_THICKNESS, MAX_SHADOW_DEPTH-RAY_BIAS
                )

                for i=1, len do
                    local item = items[i]

                    if item.id == "solid" then
                        pos[3] = math.max(pos[3], item.top+0.1)
                    end
                end

                if pos[3] > -MAX_SHADOW_DEPTH then
                    local dist = 0.7-(math.abs(pos[3] - p[3])/MAX_SHADOW_DEPTH)

                    eng.render {
                        model = mat4.from_translation(pos) * mat4.from_scale(dist),
                        mesh = assets.shadow,
                        tint = { 0, 0, 0, 180 }
                    }
                end
            end
]]

            if (has_collider_on_air or ent.floats) and not ent.no_shadow then
                local triangles = self.triangles:intersectRay(p, { 0, 0, -1 })

                if triangles[1] then
                    table.sort(triangles, function(a, b)
                        return a.t < b.t
                    end)

                    local t = triangles[1]

                    local n = vec3.add(t.p, { 0, 0, 0.1 })

                    local size = 0.7 - (t.t / 16)

                    local m = fam.triangle_to_normal(table.unpack(t))
                    local e = fam.normal_to_euler(m)

                    eng.render {
                        model = mat4.from_transform(n, e, size),
                        mesh = assets.mod_shadow,
                        tint = { 0, 0, 0, 180 }
                    }
                end
            end

            --            if ent.sphere_collider then
            --                local t = vec3.add(p, ent.sphere_collider.offset)
            --
            --                eng.render {
            --                    mesh = assets.sphere,
            --                    model = mat4.from_transform(t, {0, 0, 0}, ent.sphere_collider.size),
            --                    texture = { 0, o0, 1, 1 },
            --                    tint = { 255, 255, 255, 255 / 6 }
            --                }
            --            end

            ::continue::
        end

        eng.render {
            mesh = assets.mod_sphere,
            model = mat4.from_transform(ball.position, 0, 0.1),
            texture = { 0, 0, 1, 1 },
            tint = { 255, 0, 0, 255 }
        }

        for i = #self.particles, 1, -1 do
            local p = self.particles[i]

            p.position = {
                p.position[1] + p.velocity[1] * delta * p.life,
                p.position[2] + p.velocity[2] * delta * p.life,
                p.position[3] + p.velocity[3] * delta * p.life
            }
            p.life = p.life - (p.decay_rate * delta)

            eng.render {
                mesh = assets.mod_plane,
                model = mat4.from_transform(p.position, 0, p.scale * (p.life ^ 2)),
                texture = { 0, 0, 1, 1 },
                tint = p.color
            }

            if p.light then
                eng.light(p.position, vec3.mul_val(p.light, p.life, 3))
            end

            if p.life <= 0 then
                self.particles[i] = self.particles[#self.particles]
                self.particles[#self.particles] = nil
            end
        end

        --local str = "This is a test text! and it's great!\nhello world!\nI love you a lot!"
        --ui.print_3d(str, {-2, 10, 3}, 0xFFFFFFFF, 120)

        eng.log("particles: " .. #self.particles)

        camera.frame(delta)

        eng.render {
            mesh = self.world_mesh
        }

        eng.render {
            mesh = dome,
            model = mat4.from_transform(vec3.mul(camera.current.eye, { 1, 1, 0 }), 0, self.far - 10)
        }

        if self.interactable then
            ---- 388, 500, 12, 12
            --eng.rect(-7, -7, 14, 14, 0xFFFFFFFF)
            eng.quad({ 400, 496, 12, 12 }, -12, 100, 0xFFFFFFFF, 2, 2)
        end

        -- transition code
        local trans = self.transition
        if trans.ease == "in" then
            if not trans.a then
                --eng.sound_play(assets.snd_transition_in, { gain = 0.5 })
                trans.a = 0
            end

            trans.a = trans.a + delta * 1.2
            if trans.a >= 1.4 then
                trans.a = 1
                trans.ease = "out"
                if trans.callback then
                    trans.callback()
                end
                --eng.sound_play(assets.snd_transition_out, { gain = 0.5 })
            end
        end

        if trans.ease == "out" then
            trans.a = trans.a - delta * 0.8

            if trans.a <= 0 then
                trans.a = nil
                trans.ease = nil
            end
        end

        -- TODO: Better transition
        if trans.a then
            local i = fam.unlerp(0, 0.7, trans.a)

            local w, h = eng.size()
            w = w + 2
            h = h + 2

            local k = 1 / 4

            local t1 = fam.unlerp(k * 0, (k * 0) + k, i) ^ 2
            local t2 = fam.unlerp(k * 1, (k * 1) + k, i) ^ 2
            local t3 = fam.unlerp(k * 2, (k * 2) + k, i) ^ 2
            local t4 = fam.unlerp(k * 3, (k * 3) + k, i) ^ 2

            local m = math.max(w, h) / fam.lerp(6, 2, fam.unlerp(0.7, 1.0, trans.a) ^ 3)
            local wd = fam.lerp(0, m, (t1 + t2 + t3 + t4) / 4)

            eng.rect(-w / 2, -h / 2, w * t1, wd, { 0, 0, 0, fam.to_u8(t1) })

            eng.rect((w / 2) - wd, -h / 2, wd, h * t2, { 0, 0, 0, fam.to_u8(t2) })

            local nw = w * t3
            eng.rect((w / 2) - nw, (h / 2) - wd, nw, wd, { 0, 0, 0, fam.to_u8(t3) })

            local nh = h * t4
            eng.rect(-w / 2, (h / 2) - nh, wd, nh, { 0, 0, 0, fam.to_u8(t4) })
        end

        dialog.frame(delta)

        --eng.rect((-w/2)+m, y-m+4, 160, 160, 0xFF000066)
    end,
}

return state
