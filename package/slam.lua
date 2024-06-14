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
]]

local vec3   = require("lib.vec3")
local slam = {}

local clamp = function(a, min, max)
    return math.max(min, math.min(max, a))
end

local signed_distance = function(plane, base)
    local d = -vec3.dot(plane.normal, plane.position)
    return vec3.dot(base, plane.normal) + d
end

local swap = function(a, b)
    return b, a
end


local u, v, w = {}, {}, {}
local vw, vu = {}, {}
local uw, uv = {}, {}
slam.triangle_intersects_point = function(point, v0, v1, v2)
    vec3.sub(v1, v0, u)
    vec3.sub(v2, v0, v)
    vec3.sub(point, v0, w)

    vec3.cross(v, w, vw)
    vec3.cross(v, u, vu)

    if (vec3.dot(vw, vu) < 0) then
        return false
    end

    vec3.cross(u, w, uw)
    vec3.cross(u, v, uv)

    if (vec3.dot(uw, uv) < 0) then
        return false
    end

    local d = 1 / vec3.length(uv)
    local r = vec3.length(vw) * d
    local t = vec3.length(uw) * d

    return (r + t) <= 1
end


local get_lowest_root = function(a, b, c, max)
    local determinant = b * b - 4 * a * c

    if determinant < 0 then
        return false
    end

    local sqrtd = math.sqrt(determinant)
    local r1 = (-b - sqrtd) / (2 * a)
    local r2 = (-b + sqrtd) / (2 * a)

    if (r1 > r2) then -- perform swap
        r1, r2 = swap(r1, r2)
    end

    if (r1 > 0 and r1 < max) then
        return r1
    end

    if (r2 > 0 and r2 < max) then
        return r2
    end

    return false
end

local plane_intersect = {}
local plane_normal = {}
local ba, ca = {}, {}
local check_triangle = function(packet, p1, p2, p3, id)
    vec3.sub(p2, p1, ba)
    vec3.sub(p3, p1, ca)
    vec3.normalize(vec3.cross(ba, ca), plane_normal)

    --  // only check front facing triangles
    --	if (vec3.dot(pn, packet.e_norm_velocity) > 0.0) {
    --		//return packet;
    --	}

    local t0 = 0
    local embedded_in_plane = false

    local signed_dist_to_plane = vec3.dot(packet.e_base_point, plane_normal) - vec3.dot(plane_normal, p1)
    local normal_dot_vel = vec3.dot(plane_normal, packet.e_velocity)

    if normal_dot_vel == 0 then
        embedded_in_plane = true
        t0 = 0

        if math.abs(signed_dist_to_plane) >= 1 then
            return packet
        end
    else
        local nvi = 1 / normal_dot_vel

        t0 = (-1 - signed_dist_to_plane) * nvi
        local t1 = (1 - signed_dist_to_plane) * nvi

        if (t0 > t1) then
            t0, t1 = swap(t0, t1)
        end

        if (t0 > 1 or t1 < 0) then
            return packet
        end

        t0 = clamp(t0, 0, 1)
    end

    local collision_point = { 0, 0, 0 }
    local found_collision = false
    local t = 1

    if not embedded_in_plane then
        vec3.sub(packet.e_base_point, plane_normal, plane_intersect)
        local temp = packet.e_velocity * t0
        vec3.add(plane_intersect, temp, plane_intersect)

        if slam.triangle_intersects_point(plane_intersect, p1, p2, p3) then
            found_collision = true
            t = t0
            collision_point = plane_intersect
        end
    end

    if not found_collision then
        local velocity_sq_length = vec3.length2(packet.e_velocity)
        local a = velocity_sq_length

        -- TODO: CHECK IF FUCKED
        local function check_point(p)
            local b = 2 * vec3.dot(packet.e_velocity, packet.e_base_point - p)
            local c = vec3.length2(p - packet.e_base_point) - 1

            local new_t = get_lowest_root(a, b, c, t)

            if new_t then
                t = new_t
                found_collision = true
                return p
            end
        end

        collision_point =
            check_point(p1) or
            check_point(p2) or
            check_point(p3) or
            collision_point

        local function check_edge(pa, pb)
            local edge = vec3.sub(pb, pa)
            local base_to_vertex = vec3.sub(pa, packet.e_base_point)

            local edge_sq_length = vec3.length2(edge)
            local edge_dot_velocity = vec3.dot(edge, packet.e_velocity)
            local edge_dot_base_to_vertex = vec3.dot(edge, base_to_vertex)

            local a = edge_sq_length * -velocity_sq_length + edge_dot_velocity * edge_dot_velocity

            local b = edge_sq_length * (2.0 * vec3.dot(packet.e_velocity, base_to_vertex)) -
                2.0 * edge_dot_velocity * edge_dot_base_to_vertex

            local c = edge_sq_length * (1.0 - vec3.length2(base_to_vertex)) +
                edge_dot_base_to_vertex * edge_dot_base_to_vertex;

            local new_t = get_lowest_root(a, b, c, t)
            if new_t then
                local f = (edge_dot_velocity * new_t - edge_dot_base_to_vertex) / edge_sq_length
                if (f >= 0 and f <= 1) then
                    t = new_t
                    found_collision = true
                    return pa + (edge * f)
                end
            end
        end

        collision_point =
            check_edge(p1, p2) or
            check_edge(p2, p3) or
            check_edge(p3, p1) or
            collision_point
    end

    if found_collision then
        local dist_to_coll = t * vec3.length(packet.e_velocity)

        if (not packet.found_collision or dist_to_coll < packet.nearest_distance) then
            packet.nearest_distance = dist_to_coll
            packet.intersect_point = collision_point
            packet.intersect_time = t
            packet.found_collision = true
            packet.id = id
        end
    end

    return packet
end

local check_collision = function(packet, triangles, ids)
    local inv_radius = packet.e_inv_radius

    for index, triangle in ipairs(triangles) do
        local v0 = triangle[1]
        local v1 = triangle[2]
        local v2 = triangle[3]

        check_triangle(
            packet,
            vec3.mul(v0, inv_radius),
            vec3.mul(v1, inv_radius),
            vec3.mul(v2, inv_radius),
            ids and ids[index] or 0
        )
    end
end

-- This implements the improvements to Kasper Fauerby's "Improved Collision
-- detection and Response" proposed by Jeff Linahan's "Improving the Numerical
-- Robustness of Sphere Swept Collision Detection"
local VERY_CLOSE_DIST = 0.00125

slam.collide_with_world = function(packet, position, velocity, triangles, ids)
    local first_plane
    local dest = position + velocity
    local speed = 1

    for i = 1, 3 do
        packet.e_norm_velocity = vec3.normalize(velocity)
        packet.e_velocity = vec3.copy(velocity)
        packet.e_base_point = vec3.copy(position)
        packet.found_collision = false
        packet.nearest_distance = 1e20

        check_collision(packet, triangles, ids)

        if not packet.found_collision then
            return dest
        end

        local touch_point = vec3.add(position, vec3.mul_val(velocity, packet.intersect_time))

        local pn = vec3.normalize(vec3.sub(touch_point, packet.intersect_point))
        local p = {
            position = packet.intersect_point,
            normal = pn
        }
        local n = vec3.normalize(vec3.mul(p.normal, packet.e_radius))

        local dist = vec3.length(velocity) * packet.intersect_time
        local short_dist = math.max(dist - speed * VERY_CLOSE_DIST, 0)
        local nvel = vec3.normalize(velocity)
        vec3.add(position, vec3.mul_val(nvel, short_dist), position)

        table.insert(packet.contacts, {
            id = packet.id,
            position = vec3.mul(p.position, packet.e_radius),
            normal = n,
            near = packet.intersect_time
        })

        if i == 1 then
            local long_radius = 1 + speed * VERY_CLOSE_DIST
            first_plane = p

            vec3.sub(dest, vec3.mul_val(first_plane.normal, signed_distance(first_plane, dest) - long_radius), dest)
            velocity = vec3.sub(dest, position)

        elseif i == 2 and first_plane then
            local second_plane = p
            local crease = vec3.normalize(vec3.cross(first_plane.normal, second_plane.normal))
            local dis = vec3.dot(vec3.sub(dest, position), crease)
            velocity = vec3.add_val(crease, -dis)
            dest = vec3.add(position, velocity)

        end
    end

    return position
end

local function get_tris(position, velocity, radius, query, data)
    local scale = math.max(1.5, vec3.length(velocity)) * 1.25
    local r3_position = position
    local query_radius = radius * scale
    local min = r3_position - query_radius
    local max = r3_position + query_radius

    return query(min, max, velocity, data)
end

local function sub_update(packet, position, triangles, ids)
    packet.e_velocity = packet.e_velocity * 0.5

    local e_position = vec3.copy(packet.e_position)
    local e_velocity = vec3.copy(packet.e_velocity)

    local final_position = slam.collide_with_world(packet, e_position, e_velocity, triangles, ids)

    packet.r3_position = final_position * packet.e_radius
    packet.r3_velocity = packet.r3_position - position
end

-- query must be function(min, max, velocity)->triangles,id?
-- returns position, velocity, contacts (as planes)
slam.check = function(position, velocity, radius, query, substeps, data)
    substeps = substeps or 1
    velocity = vec3.div(velocity, substeps)

    local _q = query
    if type(query) == "table" then
        query = function()
            return _q
        end
    end

    local tri_cache, id_cache = get_tris(position, velocity, radius, query, data)

    local base = position
    local contacts = {}
    for i = 1, substeps do
        local inv_r = 1 / radius

        local packet = {
            r3_position      = position,
            r3_velocity      = velocity,
            e_radius         = radius,
            e_inv_radius     = {inv_r, inv_r, inv_r},
            e_position       = vec3.mul_val(position, radius),
            e_velocity       = vec3.mul_val(velocity, radius),
            e_norm_velocity  = {0, 0, 0},
            e_base_point     = {0, 0, 0},
            found_collision  = false,
            nearest_distance = 0,
            intersect_point  = {0, 0, 0},
            intersect_time   = 0,
            id               = 0,
            contacts         = contacts
        }

        sub_update(packet, packet.r3_position, tri_cache, id_cache)
        
        position = packet.r3_position
        velocity = packet.r3_velocity
    end

    return position, position - base, contacts
end
slam.__call = slam.check

return setmetatable(slam, slam)
