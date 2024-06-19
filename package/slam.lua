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

local slam = {}


local clamp = function(a, min, max)
    return math.max(min, math.min(max, a))
end

local vec3_sub = function (a, b)
    return {
        a[1] - b[1],
        a[2] - b[2],
        a[3] - b[3],
    }
end

local vec3_add = function (a, b)
    return {
        a[1] + b[1],
        a[2] + b[2],
        a[3] + b[3],
    }
end

local vec3_mul = function (a, b)
    return {
        a[1] + b[1],
        a[2] + b[2],
        a[3] + b[3],
    }
end

local vec3_div = function (a, b)
    return {
        a[1] / b[1],
        a[2] / b[2],
        a[3] / b[3],
    }
end

local vec3_inverse = function (a)
    return {
        1 / a[1],
        1 / a[2],
        1 / a[3],
    }
end

local vec3_cross = function (a, b)
    return {
        a[2] * b[3] - a[3] * b[2],
        a[3] * b[1] - a[1] * b[3],
        a[1] * b[2] - a[2] * b[1]
    }
end

local vec3_dot = function(a, b)
	return a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
end

local vec3_length2 = function (a)
    return a[1]^2 + a[2]^2 + a[3]^2
end

local vec3_length = function (a)
	return math.sqrt(vec3_length2(a))
end

local vec3_scale = function(a, b)
	return {
        a[1] * b,
        a[2] * b,
        a[3] * b
    }
end

local vec3_normalize = function (a)
    local l = vec3_length(a)

    if l == 0 then
        return {0, 0, 0}
    end

    return {
        a[1] / l,
        a[2] / l,
        a[3] / l,
    }
end

local vec3_copy = function (a)
    return { table.unpack(a) }
end


local signed_distance = function(plane, base)
    local d = -vec3_dot(plane.normal, plane.position)
    return vec3_dot(base, plane.normal) + d
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
        local tmp = r2
        r2 = r1
        r1 = tmp
    end

    if (r1 > 0 and r1 < max) then
        return r1
    end

    if (r2 > 0 and r2 < max) then
        return r2
    end

    return false
end


slam.triangle_intersects_point = function(point, v0, v1, v2)
    local u = vec3_sub(v1, v0)
    local v = vec3_sub(v2, v0)
    local w = vec3_sub(point, v0)

    local vw = vec3_cross(v, w)
    local vu = vec3_cross(v, u)

    if (vec3_dot(vw, vu) < 0) then
        return false
    end

    local uw = vec3_cross(u, w)
    local uv = vec3_cross(u, v)

    if (vec3_dot(uw, uv) < 0) then
        return false
    end

    local d = 1 / vec3_length(uv)
    local r = vec3_length(vw) * d
    local t = vec3_length(uw) * d

    return (r + t) <= 1
end

local check_triangle = function(packet, p1, p2, p3, id)
    local plane_normal = vec3_normalize(vec3_cross(vec3_sub(p2, p1), vec3_sub(p3, p1)))

    --  // only check front facing triangles
    --	if (vec3_dot(pn, packet.e_norm_velocity) > 0.0) {
    --		//return packet;
    --	}

    local t0 = 0
    local embedded_in_plane = false

    local signed_dist_to_plane = vec3_dot(packet.e_base_point, plane_normal) - vec3_dot(plane_normal, p1)
    local normal_dot_vel = vec3_dot(plane_normal, packet.e_velocity)

    if normal_dot_vel == 0 then
        if math.abs(signed_dist_to_plane) >= 1 then
            return packet
        end

        t0 = 0
        embedded_in_plane = true
    else
        local nvi = 1 / normal_dot_vel

        t0 = (-1 - signed_dist_to_plane) * nvi
        local t1 = (1 - signed_dist_to_plane) * nvi

        if (t0 > t1) then
            local tmp = t1
            t1 = t0
            t0 = tmp
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
        local plane_intersect = vec3_sub(packet.e_base_point, plane_normal)
        local temp = vec3_scale(packet.e_velocity, t0)
        plane_intersect = vec3_add(plane_intersect, temp)

        if slam.triangle_intersects_point(plane_intersect, p1, p2, p3) then
            found_collision = true
            t = t0
            collision_point = plane_intersect
        end
    end

    if not found_collision then
        local velocity_sq_length = vec3_length2(packet.e_velocity)
        local a = velocity_sq_length

        local function check_point(collision_point, p)
            local b = 2 * vec3_dot(packet.e_velocity, vec3_sub(packet.e_base_point, p))
            local c = vec3_length2(vec3_sub(p, packet.e_base_point)) - 1

            local new_t = get_lowest_root(a, b, c, t)

            if new_t then
                t = new_t
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

            local b = edge_sq_length * (2.0 * vec3_dot(packet.e_velocity, base_to_vertex)) -
                2.0 * edge_dot_velocity * edge_dot_base_to_vertex

            local c = edge_sq_length * (1.0 - vec3_length2(base_to_vertex)) +
                edge_dot_base_to_vertex * edge_dot_base_to_vertex;

            local new_t = get_lowest_root(a, b, c, t)

            if new_t then
                local f = (edge_dot_velocity * new_t - edge_dot_base_to_vertex) / edge_sq_length
                if (f >= 0 and f <= 1) then
                    t = new_t
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
        local dist_to_coll = t * vec3_length(packet.e_velocity)

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

-- This implements the improvements to Kasper Fauerby's "Improved Collision
-- detection and Response" proposed by Jeff Linahan's "Improving the Numerical
-- Robustness of Sphere Swept Collision Detection"
local VERY_CLOSE_DIST = 0.000125

slam.collide_with_world = function(packet, position, velocity, triangles, ids)
    local first_plane
    local dest = vec3_add(position, velocity)
    local speed = 1

    for i = 1, 3 do
        packet.e_velocity = vec3_copy(velocity)
        packet.e_base_point = vec3_copy(position)
        packet.found_collision = false
        packet.nearest_distance = 1e20

        for index, triangle in ipairs(triangles) do
            check_triangle(
                packet,
                vec3_div(triangle[1], packet.e_radius),
                vec3_div(triangle[2], packet.e_radius),
                vec3_div(triangle[3], packet.e_radius),
                ids and ids[index] or 0
            )
        end

        if not packet.found_collision then
            return dest
        end

        local touch_point = vec3_add(position, vec3_scale(velocity, packet.intersect_time))

        local pn = vec3_normalize(vec3_sub(touch_point, packet.intersect_point))
        local p = {
            position = packet.intersect_point,
            normal = pn
        }
        local n = vec3_normalize(vec3_div(p.normal, packet.e_radius))

        local dist = vec3_length(velocity) * packet.intersect_time
        local short_dist = math.max(dist - speed * VERY_CLOSE_DIST, 0)
        local nvel = vec3_normalize(velocity)
        position = vec3_add(position, vec3_scale(nvel, short_dist))

        table.insert(packet.contacts, {
            id = packet.id,
            position = vec3_mul(p.position, packet.e_radius),
            normal = n,
            t = packet.intersect_time
        })

        if i == 1 then
            local long_radius = 1 + speed * VERY_CLOSE_DIST
            first_plane = p

            dest = vec3_sub(dest,
                vec3_scale(
                    first_plane.normal,
                    signed_distance(first_plane, dest) - long_radius
                )
            )
            velocity = vec3_sub(dest, position)

        elseif i == 2 and first_plane then
            local second_plane = p
            local crease = vec3_normalize(vec3_cross(first_plane.normal, second_plane.normal))
            local dis = vec3_dot(vec3_sub(dest, position), crease)
            velocity = vec3_scale(crease, dis)
            dest = vec3_add(position, velocity)

        end
    end

    return position
end

local get_tris = function (position, velocity, radius, query)
    local scale = math.max(1.5, vec3_length(velocity)) * 1.25
    local r3_position = position
    local query_radius = vec3_scale(radius, scale)
    local min = vec3_sub(r3_position, query_radius)
    local max = vec3_add(r3_position, query_radius)

    return query(min, max)
end

-- query must be function(min, max, velocity)->triangles,id?
-- returns position, velocity, contacts (as planes)
slam.check = function(position, velocity, radius, query, substeps)
    substeps = substeps or 1

    if tonumber(radius) then
        radius = {radius, radius, radius}
    end

    local tri_cache, id_cache = get_tris(position, velocity, radius, query)
    
    local packet = {
        r3_position  = position,
        r3_velocity  = vec3_scale(velocity, 1/substeps),

        e_radius     = radius,
        e_position   = vec3_div(position, radius),
 
        contacts     = {}
    }

    for i = 1, substeps do
        packet.e_velocity   = vec3_div(packet.r3_velocity, packet.e_radius)
        packet.e_base_point = { 0, 0, 0 }

        packet.found_collision  = false
        packet.intersect_point  = { 0, 0, 0 }
        packet.intersect_time   = 0
        packet.nearest_distance = 0
        packet.id               = 0

        packet.e_position = slam.collide_with_world (
            packet,
            packet.e_position,
            packet.e_velocity,
            tri_cache,
            id_cache
        )
    
        packet.r3_position = vec3_mul(packet.e_position, packet.e_radius)
        packet.r3_velocity = vec3_sub(packet.r3_position, position)
    end

    return packet.r3_position, packet.r3_velocity, packet.contacts
end

slam.__call = slam.check

return setmetatable(slam, slam)
