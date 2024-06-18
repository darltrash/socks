local vec3 = {}

local tmp_out = {}

vec3.cross = function (a, b, out)
    assert(#a == 3)
    out = out or {}
    
    out[1] = a[2] * b[3] - a[3] * b[2]
    out[2] = a[3] * b[1] - a[1] * b[3]
    out[3] = a[1] * b[2] - a[2] * b[1]
    
    return out
end

vec3.add = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] + b[x]
    end
    return out
end

vec3.add_val = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] + b
    end
    return out
end

vec3.sub = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] - b[x]
    end
    return out
end

vec3.mul = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] * b[x]
    end
    return out
end

vec3.div = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] / b[x]
    end
    return out
end

vec3.mul_val = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] * b
    end
    return out
end


vec3.div_val = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] / b
    end
    return out
end


vec3.dot = function(a, b)
	local sum = 0
	for x=1, #a do
		sum = sum + (a[x] * b[x])
    end
	return sum
end

vec3.inverse = function (a, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = 1 / a[x]
    end
    return out
end

vec3.length2 = function (a)
    local e = 0

    for x=1, #a do
        e = e + a[x]*a[x]
    end
    
	return e
end

vec3.length = function (a)
	return math.sqrt(vec3.length2(a))
end


vec3.distance = function (a, b, len)
    return vec3.length(vec3.sub(a, b, len))
end

vec3.normalize = function (a, out)
    out = out or {}

    local len = vec3.length(a)

	if (len == 0.0) then
		for i=1, #a do
			out[i] = 0.0
        end
	else
        for i=1, #a do
			out[i] = a[i] / len
        end
    end

    return out
end

local lerp = function (a, b, t)
    return a * (1 - t) + b * t
end

vec3.lerp = function(a, b, t)
    local out = {}
	for x=1, #a do
		out[x] = lerp(a[x], b[x], t)
    end
    return out
end

vec3.copy = function (a)
    return {table.unpack(a)}
end

vec3.random = function (len)
    local r = {}

    for x=1, len do
        r[x] = math.random(-100, 100)
    end

    return vec3.normalize(r)
end

vec3.min = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = math.min(a[x], b[x])
    end
    return out
end

vec3.max = function (a, b, len, out)
    out = out or {}
    len = len or #a
    for x=1, len do
        out[x] = math.max(a[x], b[x])
    end
    return out
end

vec3.zero2 = {0, 0}
vec3.zero3 = {0, 0, 0}

return vec3