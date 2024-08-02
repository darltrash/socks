local materials = {}

local texture = eng.load_image('assets/tex_material.png')

materials.lookup = {
    [0xFF0000FF] = { -- "RED" (bricc)
        collides = true,
    },

    [0xFF006AFF] = { -- "ORANGE" (wood)
        collides = true,
    },

    [0xFF00E2FF] = { -- "YELLOW"
        collides = true,
    },

    [0xFF00FF97] = { -- "GREEN" (grass (ass))
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

materials.from_uv = function(u, v)
    local x = math.floor(((u % 1) * texture.w) + 0.1)
    local y = math.floor(((v % 1) * texture.h) + 0.1)

    local i = (y * texture.w) + x

    local m = string.unpack("<I4", texture.pixels, (i * 4) + 1)

    local l = materials.lookup[m]
    if not l then
        l = materials.lookup[0]
    end

    return l
end


return materials
