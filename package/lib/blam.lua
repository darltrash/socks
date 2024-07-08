-- SLAM: Sirno's Lerfect Allision Mibrary

--[[
    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>
]]

--[[
    This library was made with the intention to continue the works done at:
        https://github.com/excessive/cpcl

    Please read https://github.com/darltrash/lumi/blob/main/docs/slam.md
    to get a better knowledge of what this can and cannot do

    CHANGELOG:
        - Removed lumi.vec3 dependency
        - Cleaned up the code
        - Fixed an internal bug
]]

-- HUGE THANKS TO SHAKESODA

-- This implements the improvements to Kasper Fauerby's "Improved Collision
-- detection and Response" proposed by Jeff Linahan's "Improving the Numerical
-- Robustness of Sphere Swept Collision Detection"

local blam = {}

local function vec3_sub(a, b)
    return { a[1] - b[1], a[2] - b[2], a[3] - b[3] }
end

local function vec3_add(a, b)
    return { a[1] + b[1], a[2] + b[2], a[3] + b[3] }
end

local function vec3_mul(a, b)
    return { a[1] * b[1], a[2] * b[2], a[3] * b[3] }
end

local function vec3_div(a, b)
    return { a[1] / b[1], a[2] / b[2], a[3] / b[3] }
end

local function vec3_inverse(a)
    return { 1 / a[1], 1 / a[2], 1 / a[3] }
end

local function vec3_scale(a, b)
    return { a[1] * b, a[2] * b, a[3] * b }
end

local function vec3_cross(a, b)
    return {
        a[2] * b[3] - a[3] * b[2],
        a[3] * b[1] - a[1] * b[3],
        a[1] * b[2] - a[2] * b[1]
    }
end

local function vec3_dot(a, b)
    return
        a[1] * b[1] +
        a[2] * b[2] +
        a[3] * b[3]
end

local function vec3_length2(a)
    return a[1] * a[1] + a[2] * a[2] + a[3] * a[3]
end

local function vec3_length(a)
    return math.sqrt(vec3_length2(a))
end

local function vec3_clone(a)
    return { a[1], a[2], a[3] }
end

local function vec3_normalize(a)
    local l = vec3_length(a)
    if l == 0 then
        return { 0, 0, 0 }
    end
    return { a[1] / l, a[2] / l, a[3] / l }
end

local function clamp(v, l, h)
    return math.max(l, math.min(h, v))
end

local function signed_distance(plane, base_point)
    local d = -vec3_dot(plane.normal, plane.position)
    return vec3_dot(base_point, plane.normal) + d
end

local function triangle_intersects_point(point, v0, v1, v2)
    local u = vec3_sub(v1, v0)
    local v = vec3_sub(v2, v0)
    local w = vec3_sub(point, v0)
    local vw = vec3_cross(v, w)
    local vu = vec3_cross(v, u)
    
    if vec3_dot(vw, vu) < 0 then
        return false
    end

    local uw = vec3_cross(u, w)
    local uv = vec3_cross(u, v)
    if vec3_dot(uw, uv) < 0 then
        return false
    end

    local d = 1 / vec3_length(uv)
    local r = vec3_length(vw) * d
    local t = vec3_length(uw) * d
    return r + t <= 1
end

local function get_lowest_root(root, a, b, c, max)
    local determinant = b * b - 4 * a * c

    if determinant < 0 then
        return false
    end

    local sqrtD = math.sqrt(determinant)
    local r1 = (-b - sqrtD) / (2 * a)
    local r2 = (-b + sqrtD) / (2 * a)

    if r1 > r2 then
        local temp = r2
        r2 = r1
        r1 = temp
    end

    if r1 > 0 and r1 < max then
        root.v = r1
        return true
    end

    if r2 > 0 and r2 < max then
        root.v = r2
        return true
    end
    
    return false
end

local function check_triangle(packet, p1, p2, p3, id)
    local plane_normal = vec3_normalize(vec3_cross(vec3_sub(p2, p1), vec3_sub(p3, p1)))

    -- if vec3_dot(pn, packet.e_norm_velocity) > 0 then
    -- end

    local t0 = 0
    local embedded_in_plane = false
    local signed_dist_to_plane = vec3_dot(packet.e_base_point, plane_normal) - vec3_dot(plane_normal, p1)
    local normal_dot_vel = vec3_dot(plane_normal, packet.e_velocity)

    if math.abs(normal_dot_vel) < 0.001 then
        if math.abs(signed_dist_to_plane) >= 1 then
            return packet
        else
            embedded_in_plane = true
            t0 = 0
        end
    else
        local nvi = 1 / normal_dot_vel
        t0 = (-1 - signed_dist_to_plane) * nvi
        local t1 = (1 - signed_dist_to_plane) * nvi
        
        if t0 > t1 then
            local temp = t1
            t1 = t0
            t0 = temp
        end
        
        if t0 > 1 or t1 < 0 then
            return packet
        end
        
        t0 = clamp(t0, 0, 1)
    end

    local collision_point = { 0, 0, 0 }
    local found_collision = false
    local t = 1

    if not embedded_in_plane then
        local plane_intersect = vec3_sub(packet.e_base_point, plane_normal)
        local temp = vec3_scale(packet.e_velocity, t0)
        plane_intersect = vec3_add(plane_intersect, temp)
        if triangle_intersects_point(plane_intersect, p1, p2, p3) then
            found_collision = true
            t = t0
            collision_point = plane_intersect
        end
    end

    if not found_collision then
        local velocity_sq_length = vec3_length2(packet.e_velocity)
        local a = velocity_sq_length
        local new_t = { v = 0 }

        local function check_point(collision_point, p)
            local b = 2 * vec3_dot(packet.e_velocity, vec3_sub(packet.e_base_point, p))
            local c = vec3_length2(vec3_sub(p, packet.e_base_point)) - 1
            if get_lowest_root(new_t, a, b, c, t) then
                t = new_t.v
                found_collision = true
                collision_point = p
            end
            return collision_point
        end

        collision_point = check_point(collision_point, p1)
        
        if not found_collision then
            collision_point = check_point(collision_point, p2)
        end
        
        if not found_collision then
            collision_point = check_point(collision_point, p3)
        end
        
        local function check_edge(collision_point, pa, pb)
            local edge = vec3_sub(pb, pa)
            local base_to_vertex = vec3_sub(pa, packet.e_base_point)
            local edge_sq_length = vec3_length2(edge)
            local edge_dot_velocity = vec3_dot(edge, packet.e_velocity)
            local edge_dot_base_to_vertex = vec3_dot(edge, base_to_vertex)
            
            local a = edge_sq_length * -velocity_sq_length + edge_dot_velocity * edge_dot_velocity

            local b =
                edge_sq_length * (2 * vec3_dot(packet.e_velocity, base_to_vertex)) -
                2 * edge_dot_velocity * edge_dot_base_to_vertex
            
            local c =
                edge_sq_length * (1 - vec3_length2(base_to_vertex)) + edge_dot_base_to_vertex * edge_dot_base_to_vertex

            if get_lowest_root(new_t, a, b, c, t) then
                local f = (edge_dot_velocity * new_t.v - edge_dot_base_to_vertex) / edge_sq_length
                if f >= 0 and f <= 1 then
                    t = new_t.v
                    found_collision = true
                    collision_point = vec3_add(pa, vec3_scale(edge, f))
                end
            end

            return collision_point
        end
        
        collision_point = check_edge(collision_point, p1, p2)
        collision_point = check_edge(collision_point, p2, p3)
        collision_point = check_edge(collision_point, p3, p1)
    end
    
    if found_collision then
        t = t
        local dist_to_coll = t * vec3_length(packet.e_velocity)
        if not packet.found_collision or dist_to_coll < packet.nearest_distance then
            packet.nearest_distance = dist_to_coll
            packet.intersect_point = collision_point
            packet.intersect_time = t
            packet.found_collision = true
            packet.id = id
        end
    end

    return packet
end

local function check_collision(packet, tris, ids)
    local inv_radius = packet.e_inv_radius

    for i=1, #tris, 3 do
        local v0 = tris[i + 0]
        local v1 = tris[i + 1]
        local v2 = tris[i + 2]

        check_triangle(
            packet,
            vec3_mul(v0, inv_radius),
            vec3_mul(v1, inv_radius),
            vec3_mul(v2, inv_radius),
            ids and ids[i] or 0
        )
    end
end

local function response_get_tris(position, velocity, radius, query)
    local scale = math.max(1.5, vec3_length(velocity)) * 1.5
    local query_radius = vec3_scale(radius, scale)
    local min = vec3_sub(position, query_radius)
    local max = vec3_add(position, query_radius)

    local tri_cache = {}
    query(tri_cache, min, max, velocity)
    return tri_cache
end

local VERY_CLOSE_DIST = 0.00125

local function collide_with_world(packet, e_position, e_velocity, tris, ids)
    local first_plane = nil
    local dest = vec3_add(e_position, e_velocity)
    local speed = 1

    for i=1, 3 do
        packet.e_norm_velocity = vec3_clone(e_velocity)
        packet.e_norm_velocity = vec3_normalize(packet.e_norm_velocity)
        packet.e_velocity = vec3_clone(e_velocity)
        packet.e_base_point = vec3_clone(e_position)
        packet.found_collision = false
        packet.nearest_distance = 100000000000000000000
        
        check_collision(packet, tris, ids)
        
        if not packet.found_collision then
            return dest
        end

        local touch_point = vec3_add(e_position, vec3_scale(e_velocity, packet.intersect_time))

        local p = {
            position = packet.intersect_point,
            normal = vec3_normalize(vec3_sub(touch_point, packet.intersect_point))
        }

        local n = vec3_normalize(vec3_div(p.normal, packet.e_radius))
        local dist = vec3_length(e_velocity) * packet.intersect_time
        local short_dist = math.max(dist - speed * VERY_CLOSE_DIST, 0)
        local nvel = vec3_normalize(e_velocity)

        e_position = vec3_add(e_position, vec3_scale(nvel, short_dist))

        table.insert(packet.contacts, {
            id = packet.id,
            position = vec3_mul(p.position, packet.e_radius),
            normal = n
        })
        
        if i == 1 then
            local long_radius = 1 + speed * VERY_CLOSE_DIST
            first_plane = p
            dest = vec3_sub(dest, vec3_scale(first_plane.normal, signed_distance(first_plane, dest) - long_radius))
            e_velocity = vec3_sub(dest, e_position)

        elseif i == 2 and first_plane then
            local second_plane = p
            local crease = vec3_normalize(vec3_cross(first_plane.normal, second_plane.normal))
            local dis = vec3_dot(vec3_sub(dest, e_position), crease)
            e_velocity = vec3_scale(crease, dis)
            dest = vec3_add(e_position, e_velocity)
        end
    end

    return e_position
end

function blam.response_check(tri_cache, id_cache, position, velocity, radius, query)
    local packet = {
        r3_position = position,
        r3_velocity = velocity,
        e_radius = radius,
        e_inv_radius = vec3_inverse(radius),
        e_position = vec3_div(position, radius),
        e_velocity = vec3_div(velocity, radius),
        e_norm_velocity = { 0, 0, 0 },
        e_base_point = { 0, 0, 0 },
        found_collision = false,
        nearest_distance = 0,
        intersect_point = { 0, 0, 0 },
        intersect_time = 0,
        id = 0,
        contacts = {}
    }

    local r3_position = vec3_mul(packet.e_position, packet.e_radius)
    local query_radius = packet.e_radius
    local min = vec3_sub(r3_position, query_radius)
    local max = vec3_add(r3_position, query_radius)

    if query then
        tri_cache = response_get_tris(min, max, packet.r3_velocity, query)
        id_cache = nil
        if #tri_cache > 0 and #tri_cache % 3 ~= 0 then
            error("tri cache must be filled with a multiple of 3 values (v0, v1, v2)", 0)
        end
    end

    check_collision(packet, tri_cache, id_cache)

    if packet.found_collision then
        local touch_point = vec3_add(
            packet.e_position,
            vec3_scale(
                packet.e_velocity,
                packet.intersect_time
            )
        )

        table.insert(packet.contacts, {
            id = packet.id,
            position = vec3_mul(packet.intersect_point, packet.e_radius),
            normal = vec3_normalize(vec3_div(vec3_sub(touch_point, packet.intersect_point), packet.e_radius))
        })
    end

    return {
        position = packet.intersect_point,
        contacts = packet.contacts
    }
end

function blam.response_update(position, velocity, radius, query, substeps)
    substeps = substeps or 1
    velocity = vec3_scale(velocity, 1 / substeps)

    local tri_cache = response_get_tris(position, velocity, radius, query)

    if #tri_cache > 0 and #tri_cache % 3 ~= 0 then
        error("tri cache must be filled with a multiple of 3 values (x, y, z)", 0)
    end

    local base_position = vec3_clone(position)

    local contacts = {}
    for i=1, substeps do
        local packet = {
            r3_position = position,
            r3_velocity = velocity,
            e_radius = radius,
            e_inv_radius = vec3_inverse(radius),
            e_position = vec3_div(position, radius),
            e_velocity = vec3_div(velocity, radius),
            e_norm_velocity = { 0, 0, 0 },
            e_base_point = { 0, 0, 0 },
            found_collision = false,
            nearest_distance = 0,
            intersect_point = { 0, 0, 0 },
            intersect_time = 0,
            id = 0,
            contacts = contacts
        }

        local e_position = vec3_clone(packet.e_position)
        local e_velocity = vec3_clone(packet.e_velocity)
    
        local final_position = collide_with_world(packet, e_position, e_velocity, tri_cache, nil)

        position = vec3_mul(final_position, packet.e_radius)
        velocity = vec3_sub(packet.r3_position, position)
    end

    return
        position,
        vec3_sub(position, base_position),
        contacts
end

return blam
