local fam = {}

fam.lerp = function(a, b, t)
    return a * (1 - t) + b * t
end

fam.decay = function(value, target, rate, delta)
    return fam.lerp(target, value, math.exp(-math.exp(rate) * delta))
end

fam.sign = function(a)
    return (a > 0) and 1 or -1
end

fam.signz = function(a)
    return (a > 0) and 1 or (a < 0) and -1 or 0
end

fam.hex = function(hex, alpha)
    local h = hex:gsub("#", "")
    return {
        (tonumber("0x" .. h:sub(1, 2)) / 255),
        (tonumber("0x" .. h:sub(3, 4)) / 255),
        (tonumber("0x" .. h:sub(5, 6)) / 255),
        alpha or 1
    }
end

fam.hsl = function (h, s, l)
    if s == 0 then return l, l, l end
    local function to(p, q, t)
        if t < 0 then t = t + 1 end
        if t > 1 then t = t - 1 end
        if t < .16667 then return p + (q - p) * 6 * t end
        if t < .5 then return q end
        if t < .66667 then return p + (q - p) * (.66667 - t) * 6 end
        return p
    end
    local q = l < .5 and l * (1 + s) or l + s - l * s
    local p = 2 * l - q
    return to(p, q, h + .33334), to(p, q, h), to(p, q, h - .33334)
end

fam.rgb2hsl = function (r, g, b)
    local max, min = math.max(r, g, b), math.min(r, g, b)
    local b = max + min
    local h = b / 2
    if max == min then return 0, 0, h end
    local s, l = h, h
    local d = max - min
    s = l > .5 and d / (2 - b) or d / b
    if max == r then h = (g - b) / d + (g < b and 6 or 0)
    elseif max == g then h = (b - r) / d + 2
    elseif max == b then h = (r - g) / d + 4
    end
    return h * .16667, s, l
end

fam.split = function(str, sep)
    local t = {}

    for s in str:gmatch("([^" .. (sep or "%s") .. "]+)") do
        table.insert(t, s)
    end

    return t
end

fam.wait = function(ticks)
    local start = eng.timer
    while (eng.timer - start) < ticks do
        coroutine.yield()
    end
end

fam.animate = function(ticks, callback) -- fn(t)
    local start = eng.timer

    local i = 0
    repeat
        i = (eng.timer - start) / ticks
        callback(i)
        coroutine.yield()
    until i >= 1
end

fam.clamp = function(x, min, max)
    return x < min and min or (x > max and max or x)
end

fam.aabb = function(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and
        x2 < x1 + w1 and
        y1 < y2 + h2 and
        y2 < y1 + h1
end

fam.point_in_cube = function (cube, point)
    local px, py, pz = table.unpack(point)
    local x, y, z, w, h, d = table.unpack(cube)

    return  px - x
        and py - y
        and pz - z
        and x + w - px
        and y + h - py
        and z + d - pz
end

fam.copy_into = function(from, into)
    for k, v in pairs(from) do
        into[k] = v
    end
end

fam.choice = function (array)
    return array[math.random(1, #array)]
end

local function shortAngleDist(a0,a1)
    local max = math.pi*2
    local da = (a1 - a0) % max
    return 2*da % max - da
end

fam.angle_lerp = function (a0,a1,t)
    return a0 + shortAngleDist(a0,a1)*t
end

fam.to_u8 = function (float)
    return math.floor(fam.clamp(float, 0, 1)*255)
end

fam.shuffle = function (t)
    local out = {}
    for i = 1, #t do
        out[i] = t[i]
    end
    for i = #out, 2, -1 do
        local j = math.random(i)
        out[i], out[j] = out[j], out[i]
    end

    return out
end

fam.array_compare = function (a, b)
    if #a ~= #b then
        return false
    end

    for i=1, #a do
        if (a[i] ~= b[i]) then
            return false
        end
    end

    return true
end

fam.loop_index = function (array, index)
    return array[(math.floor(index-1) % #array)+1]
end

fam.enum = function (array)
    for _, v in ipairs(array) do
        array[v] = v
    end
end

fam.switch = function (value)
    return function (map)
        local a = map[value] or map.default
        if type(a)=="function" then
            return a(value)
        end
        return a
    end
end

return fam
