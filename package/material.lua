local materials = {}

local texture = eng.load_image("assets/tex_material.png")

materials.lookup = {
    [0xFF0000FF] = { -- "RED"
        collides = true,
    },

    [0xFF006AFF] = { -- "ORANGE"
        collides = true,
    },

    [0xFF00E2FF] = { -- "YELLOW"
        collides = true,
    },

    [0xFF00FF97] = { -- "GREEN"
        collides = true,
    },

    [0xFFB6FF00] = { -- "CYAN"
        collides = true,
    },

    [0xFFFFA900] = { -- "SKY"
        collides = true,
    },

    [0xFFFF0044] = { -- "BLUE"
        collides = true,
    },

    [0xFFFF00CC] = { -- "PURPLE"
        collides = true,
    },

    [0xFFAD00FF] = { -- "MAGENTA"
        collides = true,
    },

    [0x0] = { -- TRANSPARENT, FALLBACK
        collides = false
    }
}

materials.from_uv = function (u, v)
    local x = math.floor(((u % 1) * texture.w) + 0.1)
    local y = math.floor(((v % 1) * texture.h) + 0.1)

    local i = (y * texture.w) + x
    
    local m = string.unpack("<I4", texture.pixels, (i*4) + 1)

    return materials.lookup[m] and m or 0
end

for i=0, 6 do
    local a = materials.from_uv(i/texture.w, 0)
    local b = materials.from_uv(i/texture.w, 1/texture.h)

    print(("%08x %08x"):format(a, b))
end

return materials