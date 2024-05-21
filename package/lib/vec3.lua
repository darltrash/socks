local vec3 = {}

vec3.add = function (a, b, len)
    local out = {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] + b[x]
    end
    return out
end

vec3.sub = function (a, b, len)
    local out = {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] - b[x]
    end
    return out
end

vec3.mul = function (a, b, len)
    local out = {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] * b[x]
    end
    return out
end

vec3.div = function (a, b, len)
    local out = {}
    len = len or #a
    for x=1, len do
        out[x] = a[x] / b[x]
    end
    return out
end

vec3.mul_val = function (a, b)
    local out = {}
    for x=1, #a do
        out[x] = a[x] * b
    end
    return out
end

vec3.div_val = function (a, b)
    local out = {}
    for x=1, #a do
        out[x] = a[x] * b
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

vec3.length = function (a)
	return math.sqrt(vec3.dot(a, a))
end

vec3.distance = function (a, b)
    return vec3.length(vec3.sub(a, b))
end

vec3.normalize = function (a)
    local out = {}
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
		out[x] = lerp(a[x], b[x], t);
    end
    return out
end

vec3.copy = function (a)
    return {table.unpack(a)}
end

return vec3