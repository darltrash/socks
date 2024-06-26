local mat4 = require "lib.mat4"
local vec3 = require "lib.vec3"
local bump = require "lib.bump"
local json = require "lib.json"
local fam  = require "lib.fam"
local bvh  = require "bvh"
local player = require "player"
local blam = require "blam"

local ui = require "ui"
local assets = require "assets"
local entities = require "entities"
local materials = require "material"

local ball = {
    position = {0, 0, 0},
    velocity = {0, 0, 0}
}

local state = {
    init = function (self, world)
        eng.ambient(0x19023dff)
        eng.far(80, 0x0c031aff)
        eng.videomode(500, 400)
        --eng.dithering(false)

        self.colliders = bump.newWorld()

        self.camera = {
            distance = 5,
            target = {0, 0, 1},
            instant = false,

            -- lerped = {0, 0, 0}
        }

        self.entities = {
            map = {},
            buffer = {},
            counter = 0
        }

        self.transition = {}
        self.teleporters = {}
        self.lights = {}
        self.particles = {}
        self.switches = {}
        self.items = {}

        self.scrunge = 0

        self.easter_time = 5
        self.easter_bunny = ""
        self.easter_eggs = {}

        world = world or "assets/lvl_hometown.exm"

        self.world_mesh = assert (
            eng.load_model(world),
            "The idiot that wrote this game had a hard time adding the right path. ("..world..")"
        )

        local vertex_format = "<fff"
        local triangles = {}

        for i=1, #self.world_mesh.data, 24 do
            local p1x, p1y, p1z = string.unpack(vertex_format, self.world_mesh.data, i+00)
            table.insert(triangles, p1x)
            table.insert(triangles, p1y)
            table.insert(triangles, p1z)
        end

        self.triangles = bvh.new(triangles)

        eng.sound_play(assets.city, { looping = true })

        eng.orientation({ -1, 0, 0 }, { 0, 0, 1 })

        self.colliders = bump.newWorld()

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
            -- todo: check type

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

            if area.type=="solid" then
                area.top = p[3] + s[3]
                self.colliders:add(area, p[1], p[2], p[3], s[1], s[2], s[3])

    --        elseif area.type=="area" and area_types[t] then
    --            table.insert(self.areas, {
    --                type = area_types[t],
    --                t = t,
    --                p[1], p[2], p[3], s[1], s[2], s[3]
    --            })

            elseif area.type=="speaker" then
                if area.tokens[2] then
                    local src = ("assets/%s.ogg"):format(area.tokens[2])
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
            table.insert(self.lights, {
                position = light.position,
                color = vec3.mul_val(light.color, light.power * math.exp(-4.2))
            })
        end
    end,

    add_entity = function (self, ent)
        if not ent.id then
            ent.id = self.entities.counter
            self.entities.counter = self.entities.counter + 1
        end
        
        local a = entities.init[ent.type]
        if a then
            a(ent, self)
        end
    
        ent.position = ent.position or {0, 0, 0}
        ent.velocity = ent.velocity or {0, 0, 0}
        ent.scale    = ent.scale    or {1, 1, 1}
        ent.rotation = ent.rotation or {0, 0, 0}
    
        ent._position = vec3.copy(ent.position)
        ent._scale    = vec3.copy(ent.scale)
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
    
    tick = function (self, timestep)
        do
            local t = eng.text()
            if #t > 0 then
                self.easter_bunny = self.easter_bunny .. t:lower()
                self.easter_time = 5
            end
    
            self.easter_time = self.easter_time - (1/30)
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

        local query_bvh = function (triangles, min, max)
            local t = self.triangles:intersectAABB({min = min, max = max})
            for k, v in ipairs(t) do
                for x=1, 3 do
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

        for x=#self.entities, 1, -1 do
            local ent = self.entities[x]
    
            local a = entities.tick[ent.type]
            if a then
                a(ent, self)
            end
    
            ent._position = vec3.copy(ent.position)
            ent._scale    = vec3.copy(ent.scale)
            ent._rotation = ent.rotation

            -- Handle some animations
            if 
                ent.texture and
                ent._texture and
                not fam.array_compare(ent._texture, ent.texture) then

                ent.texture_swap = 0.85
            end

            ent._texture  = ent.texture

            if ent.flip_x then
                ent.scale[2] = fam.lerp(ent.scale[2], ent.flip_x, 1/3)
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
                    self.velocity = {0, 0, 0}
                    ent.ghost_mode = false
                else
                    local k = {self.colliders:move(ent, t[1], t[2], t[3])}

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

--            local coll = ent.sphere_collider
--            if coll then
--                local p = vec3.add(ent.position, coll.offset)
--                local _, velocity, planes = blam.response_update(p, ent.velocity, coll.size, query_bvh)
--
--                ent.on_floor = false
--
--                for _, plane in ipairs(planes) do
--                    if vec3.dot(plane.normal, {0, 0, 1}) > 0.9 then
--                        ent.on_floor = true
--                        break
--                    end
--                end
--
--                ent.velocity = velocity
--            end

            ent.position = vec3.add(ent.position, ent.velocity)
    
            ent.interacting = false
            if p and ent.interactable and not self.transition.ease then
                local player_on_area = vec3.distance(p.position, ent.position) < (ent.area or 2)
    
                if player_on_area then
                    self.interactable = true
                    ent.interacting = eng.input("menu")==1
                end
            end
    

            -- performs a swap remove if an entity has the .delete property
            if ent.delete then
                self.entities[x] = self.entities[#self.entities]
                self.entities[#self.entities] = nil
                self.entities.map[ent.id] = nil

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
    end,

    frame = function (self, alpha, delta)
        local cam = self.camera

        for _, light in ipairs(self.lights) do
            eng.light(light.position, light.color)
        end

        for _, ent in ipairs(self.entities) do
            local visible = (ent.texture or ent.mesh) and not ent.invisible

            if not visible then goto continue end
    
            local p = vec3.lerp(ent._position, ent.position, alpha)
            local s = vec3.lerp(ent._scale,    ent.scale,    alpha)
            local r = {
                fam.angle_lerp(ent._rotation[1], ent.rotation[1], alpha),
                fam.angle_lerp(ent._rotation[2], ent.rotation[2], alpha),
                fam.angle_lerp(ent._rotation[3], ent.rotation[3], alpha),
            }
    
            if ent.camera_focus then
                cam.target = vec3.add(p, {0, 0, ent.texture[4]/26})
            end

            ent.texture_swap = fam.lerp(ent.texture_swap or 1, 1, delta * 6)
    
            local real_scale = s
            
            if not ent.mesh then
                real_scale = vec3.mul(
                    s,
                    {
                        0,
                        ent.texture[3]/16,
                        (ent.texture[4]/16) * (ent.texture_swap or 1),
                    }
                )
            end

            eng.render {
                model = mat4.from_transform(p, r, real_scale),
                mesh = ent.mesh or assets.plane,
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
                local triangles = self.triangles:intersectRay(p, {0, 0, -1})

                if triangles[1] then
                    table.sort(triangles, function (a, b)
                        return a.t < b.t
                    end)

                    local t = triangles[1]

                    local n = vec3.add(t.p, {0, 0, 0.1})

                    local size = 0.7-(t.t/16)

                    local m = fam.triangle_to_normal(table.unpack(t))
                    local e = fam.normal_to_euler(m)

                    eng.render {
                        model = mat4.from_transform(n, e, size),
                        mesh = assets.shadow,
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
--                    texture = { 0, 0, 1, 1 },
--                    tint = { 255, 255, 255, 255 / 2 }
--                }
--            end

            ::continue::
        end

        eng.render {
            mesh = assets.sphere,
            model = mat4.from_transform(ball.position, 0, 0.1),
            texture = {0, 0, 1, 1},
            tint = {255, 0, 0, 255}
        }

        for i=#self.particles, 1, -1 do
            local p = self.particles[i]

            p.position = {
                p.position[1] + p.velocity[1] * delta * p.life,
                p.position[2] + p.velocity[2] * delta * p.life,
                p.position[3] + p.velocity[3] * delta * p.life
            }
            p.life = p.life - (p.decay_rate*delta)
            
            eng.render {
                mesh = assets.plane,
                model = mat4.from_transform(p.position, 0, p.scale*(p.life^2)),
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

        do
            local eye = vec3.add (
                cam.target,

                vec3.mul_val (
                    {1, 0, 0.5},
                    cam.distance
                )
            )

            local t = cam.instant and 1 or (delta * 16)
            cam.lerped = vec3.lerp(cam.lerped or eye, eye, delta * 16)

            cam.instant = false

            eng.camera(cam.lerped, cam.target, { 0, 0, 1 })

            eng.listener(cam.target)
--
--            local w, h = eng.size()
--            local x, y = eng.mouse_position()
--
--            local r = vec3.normalize { -1, 0, 0 }
--
--            local triangles = self.triangles:intersectRay(eye, r)
--
--            for _, tri in ipairs(triangles) do
--                local str = ""
--
--                str = str .. string.pack("<fffffBBBB", tri[1]+0.1, tri[2], tri[3], 0, 0, 255, 0, 0, 255)
--                str = str .. string.pack("<fffffBBBB", tri[4]+0.1, tri[5], tri[6], 0, 0, 255, 0, 0, 255)
--                str = str .. string.pack("<fffffBBBB", tri[7]+0.1, tri[8], tri[9], 0, 0, 255, 0, 0, 255)
--                
--                eng.render {
--                    mesh = {
--                        data = str,
--                        length = 3
--                    },
--                    texture = { 0, 0, 1, 1 }
--                }
--            end
        end


        eng.render {
            mesh = self.world_mesh
        }

        if self.interactable then
            ---- 388, 500, 12, 12
            --eng.rect(-7, -7, 14, 14, 0xFFFFFFFF)
            eng.quad({400, 496, 12, 12}, -12, 100, 0xFFFFFFFF, 2, 2)
        end

        -- transition code
        local trans = self.transition
        if trans.ease == "in" then
            if not trans.a then
                eng.sound_play(assets.transition_in, { gain = 0.5 })
                trans.a = 0
            end

            trans.a = trans.a + delta * 1.2
            if trans.a >= 1.4 then
                trans.a = 1
                trans.ease = "out"
                if trans.callback then
                    trans.callback()
                end
                eng.sound_play(assets.transition_out, { gain = 0.5 })
            end
        end

        if trans.ease == "out" then
            trans.a = trans.a - delta * 0.8
            
            if trans.a <= 0 then
                trans.a = nil
                trans.ease = nil
            end
        end

        if trans.a then
            local tt = trans.a

            local vw, vh = eng.size()
            local cs = 16
            local h = false

            for x=0, vw/cs do
                for y=0, vh/cs do
                    local t = tt^2
    
                    local tx = (-vw/2)+(x*cs)
                    local ty = (-vh/2)+(y*cs)
                    local s = vec3.length{tx/vw, ty/vh}
    
                    local g = ((t*1.7)-0.5) + s
                    local r = fam.inv_square(g) * cs
                    local m = cs - r
                    
                    if h then
                        eng.rect(tx, ty+(m/2), cs, r, {0, 0, 0, fam.to_u8(g)})
                    else
                        eng.rect(tx+(m/2), ty, r, cs, {0, 0, 0, fam.to_u8(g)})
                    end
    
                    h = not h
                end
                
                h = not h
            end
        end

        eng.quad({416, 0, 16*6, 16*6}, -16*3, -16*3, {255, 255, 255, self.scrunge})

        self.scrunge = math.max(0, self.scrunge - delta * 300)
    end
}

eng.set_room(state)