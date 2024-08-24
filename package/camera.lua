local vec3 = require "lib.vec3"

local camera = {}

camera.points = {}

camera.current = {
    eye = { 0, 0, 0 },
    target = { 0, 0, 0 },
    listener = { 0, 0, 0 },

    new = true
}

---@param target table
---@param distance number
---@param speed number
---@param is_listener boolean?
---@return table
camera.focus = function(target, distance, speed, is_listener)
    local next = {
        target = target,
        distance = distance,
        speed = speed,
        is_listener = is_listener,
        dead = false,

        mode = "focus",

        coroutine = coroutine.running()
    }

    table.insert(camera.points, next)

    return next
end

camera.force_focus = function(target, distance, is_listener)
    camera.current.eye = vec3.add(
        target,
        vec3.mul_val(
            { 1, 0, 0.5 },
            distance
        )
    )

    camera.current.target = target

    if is_listener then
        camera.current.listener = target
    end
end

---@param eye table
---@param target table
---@param speed number
---@param is_listener boolean?
---@return table
camera.look_at = function(eye, target, speed, is_listener)
    local next = {
        eye = eye,
        target = target,
        speed = speed,
        dead = false,
        is_listener = is_listener,
        mode = "look_at",

        coroutine = coroutine.running()
    }

    table.insert(camera.points, next)

    return next
end

camera.frame = function(delta)
    for k, v in ipairs(camera.points) do
        if v.dead then
            table.remove(camera.points, k)
        elseif v.coroutine then
            v.dead = coroutine.status(v.coroutine) == "dead"
        end
    end

    local c = camera.points[#camera.points]
    if not c then return end

    if c.mode == "focus" then
        c.eye = vec3.add(
            c.target,
            vec3.mul_val(
                { 1, 0, 0.6 },
                c.distance
            )
        )
    end

    camera.current.eye = vec3.decay(camera.current.eye, c.eye, c.speed / 10, delta)
    camera.current.target = vec3.decay(camera.current.target, c.target, c.speed / 10, delta)

    local listener
    for x = #camera.points, 1, -1 do
        local l = camera.points[x]
        if l.is_listener then
            listener = l.target
            break
        end
    end

    if listener then
        camera.current.listener = vec3.decay(camera.current.listener, listener, c.speed / 10, delta)
    end

    eng.camera(camera.current.eye, camera.current.target, { 0, 0, 1 })
    eng.listener(camera.current.listener)
end

return camera
