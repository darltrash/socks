local assets = require "assets"
local vec3 = require "lib.vec3"
local mat4 = require "lib.mat4"
local fam = require "lib.fam"
local ui = {}

local plane = assets.mod_plane

-- Monogram by datagoblin! (datagoblin.itch.io/monogram)
ui.font = {
    ['Á'] = { 7, 192, 7, 12, ox = 1, oy = 11 },
    ['j'] = { 42, 192, 7, 11, ox = 1, oy = 8 },
    ['Ï'] = { 63, 192, 7, 11, ox = 1, oy = 10 },
    ['l'] = { 98, 223, 7, 9, ox = 1, oy = 8 },
    ['Í'] = { 21, 192, 7, 12, ox = 1, oy = 11 },
    ['É'] = { 14, 192, 7, 12, ox = 1, oy = 11 },
    ['Ó'] = { 28, 192, 7, 12, ox = 1, oy = 11 },
    ['Ä'] = { 49, 192, 7, 11, ox = 1, oy = 10 },
    ['}'] = { 47, 232, 5, 9, ox = 0, oy = 8 },
    ['r'] = { 35, 241, 7, 7, ox = 1, oy = 6 },
    ['←'] = { 91, 241, 7, 7, ox = 1, oy = 6 },
    ['t'] = { 119, 223, 7, 9, ox = 1, oy = 8 },
    ['ö'] = { 28, 232, 7, 9, ox = 1, oy = 8 },
    ['n'] = { 21, 241, 7, 7, ox = 1, oy = 6 },
    ['{'] = { 42, 232, 5, 9, ox = 0, oy = 8 },
    ['p'] = { 105, 223, 7, 9, ox = 1, oy = 6 },
    ['['] = { 64, 232, 4, 9, ox = -1, oy = 8 },
    ['e'] = { 7, 241, 7, 7, ox = 1, oy = 6 },
    [']'] = { 68, 232, 4, 9, ox = 0, oy = 8 },
    ['Ë'] = { 56, 192, 7, 11, ox = 1, oy = 10 },
    ['W'] = { 14, 223, 7, 9, ox = 1, oy = 8 },
    ['→'] = { 84, 241, 7, 7, ox = 1, oy = 6 },
    ['b'] = { 49, 223, 7, 9, ox = 1, oy = 8 },
    [' '] = { 28, 248, 3, 3, ox = 1, oy = 1 },
    ['!'] = { 72, 232, 3, 9, ox = -1, oy = 8 },
    ['"'] = { 112, 241, 5, 5, ox = 0, oy = 8 },
    ['#'] = { 78, 232, 7, 8, ox = 1, oy = 7 },
    ['$'] = { 7, 204, 7, 9, ox = 1, oy = 8 },
    ['%'] = { 14, 204, 7, 9, ox = 1, oy = 8 },
    ['&'] = { 21, 204, 7, 9, ox = 1, oy = 8 },
    ['\''] = { 121, 241, 3, 5, ox = -1, oy = 8 },
    ['('] = { 52, 232, 4, 9, ox = -1, oy = 8 },
    [')'] = { 56, 232, 4, 9, ox = 0, oy = 8 },
    ['*'] = { 88, 232, 7, 7, ox = 1, oy = 7 },
    ['+'] = { 95, 232, 7, 7, ox = 1, oy = 7 },
    [','] = { 117, 241, 4, 5, ox = 0, oy = 3 },
    ['-'] = { 14, 248, 7, 3, ox = 1, oy = 5 },
    ['.'] = { 11, 248, 3, 4, ox = -1, oy = 3 },
    ['/'] = { 28, 204, 7, 9, ox = 1, oy = 8 },
    ['0'] = { 35, 204, 7, 9, ox = 1, oy = 8 },
    ['1'] = { 42, 204, 7, 9, ox = 1, oy = 8 },
    ['2'] = { 49, 204, 7, 9, ox = 1, oy = 8 },
    ['3'] = { 56, 204, 7, 9, ox = 1, oy = 8 },
    ['4'] = { 63, 204, 7, 9, ox = 1, oy = 8 },
    ['5'] = { 70, 204, 7, 9, ox = 1, oy = 8 },
    ['6'] = { 77, 204, 7, 9, ox = 1, oy = 8 },
    ['7'] = { 84, 204, 7, 9, ox = 1, oy = 8 },
    ['8'] = { 91, 204, 7, 9, ox = 1, oy = 8 },
    ['9'] = { 98, 204, 7, 9, ox = 1, oy = 8 },
    [':'] = { 85, 232, 3, 8, ox = -1, oy = 7 },
    [';'] = { 60, 232, 4, 9, ox = 0, oy = 7 },
    ['<'] = { 102, 232, 7, 7, ox = 1, oy = 7 },
    ['='] = { 98, 241, 7, 5, ox = 1, oy = 6 },
    ['>'] = { 109, 232, 7, 7, ox = 1, oy = 7 },
    ['?'] = { 105, 204, 7, 9, ox = 1, oy = 8 },
    ['@'] = { 112, 204, 7, 9, ox = 1, oy = 8 },
    ['A'] = { 119, 204, 7, 9, ox = 1, oy = 8 },
    ['B'] = { 0, 214, 7, 9, ox = 1, oy = 8 },
    ['C'] = { 7, 214, 7, 9, ox = 1, oy = 8 },
    ['D'] = { 14, 214, 7, 9, ox = 1, oy = 8 },
    ['E'] = { 21, 214, 7, 9, ox = 1, oy = 8 },
    ['F'] = { 28, 214, 7, 9, ox = 1, oy = 8 },
    ['G'] = { 35, 214, 7, 9, ox = 1, oy = 8 },
    ['H'] = { 42, 214, 7, 9, ox = 1, oy = 8 },
    ['I'] = { 49, 214, 7, 9, ox = 1, oy = 8 },
    ['J'] = { 56, 214, 7, 9, ox = 1, oy = 8 },
    ['K'] = { 63, 214, 7, 9, ox = 1, oy = 8 },
    ['L'] = { 70, 214, 7, 9, ox = 1, oy = 8 },
    ['M'] = { 77, 214, 7, 9, ox = 1, oy = 8 },
    ['N'] = { 84, 214, 7, 9, ox = 1, oy = 8 },
    ['O'] = { 91, 214, 7, 9, ox = 1, oy = 8 },
    ['P'] = { 98, 214, 7, 9, ox = 1, oy = 8 },
    ['Q'] = { 84, 192, 7, 10, ox = 1, oy = 8 },
    ['R'] = { 105, 214, 7, 9, ox = 1, oy = 8 },
    ['S'] = { 112, 214, 7, 9, ox = 1, oy = 8 },
    ['T'] = { 119, 214, 7, 9, ox = 1, oy = 8 },
    ['U'] = { 0, 223, 7, 9, ox = 1, oy = 8 },
    ['V'] = { 7, 223, 7, 9, ox = 1, oy = 8 },
    ['Ö'] = { 70, 192, 7, 11, ox = 1, oy = 10 },
    ['X'] = { 21, 223, 7, 9, ox = 1, oy = 8 },
    ['Y'] = { 28, 223, 7, 9, ox = 1, oy = 8 },
    ['Z'] = { 35, 223, 7, 9, ox = 1, oy = 8 },
    ['Ú'] = { 35, 192, 7, 12, ox = 1, oy = 11 },
    ['\\'] = { 42, 223, 7, 9, ox = 1, oy = 8 },
    ['Ü'] = { 77, 192, 7, 11, ox = 1, oy = 10 },
    ['^'] = { 105, 241, 7, 5, ox = 1, oy = 8 },
    ['_'] = { 21, 248, 7, 3, ox = 1, oy = 2 },
    ['`'] = { 7, 248, 4, 4, ox = 0, oy = 8 },
    ['a'] = { 116, 232, 7, 7, ox = 1, oy = 6 },
    ['á'] = { 98, 192, 7, 10, ox = 1, oy = 9 },
    ['c'] = { 0, 241, 7, 7, ox = 1, oy = 6 },
    ['d'] = { 56, 223, 7, 9, ox = 1, oy = 8 },
    ['ä'] = { 7, 232, 7, 9, ox = 1, oy = 8 },
    ['f'] = { 63, 223, 7, 9, ox = 1, oy = 8 },
    ['g'] = { 70, 223, 7, 9, ox = 1, oy = 6 },
    ['h'] = { 77, 223, 7, 9, ox = 1, oy = 8 },
    ['i'] = { 84, 223, 7, 9, ox = 1, oy = 8 },
    ['é'] = { 105, 192, 7, 10, ox = 1, oy = 9 },
    ['k'] = { 91, 223, 7, 9, ox = 1, oy = 8 },
    ['ë'] = { 14, 232, 7, 9, ox = 1, oy = 8 },
    ['m'] = { 14, 241, 7, 7, ox = 1, oy = 6 },
    ['í'] = { 112, 192, 7, 10, ox = 1, oy = 9 },
    ['o'] = { 28, 241, 7, 7, ox = 1, oy = 6 },
    ['ï'] = { 21, 232, 7, 9, ox = 1, oy = 8 },
    ['q'] = { 112, 223, 7, 9, ox = 1, oy = 6 },
    ['ñ'] = { 91, 192, 7, 10, ox = 1, oy = 9 },
    ['s'] = { 42, 241, 7, 7, ox = 1, oy = 6 },
    ['ó'] = { 119, 192, 7, 10, ox = 1, oy = 9 },
    ['u'] = { 49, 241, 7, 7, ox = 1, oy = 6 },
    ['v'] = { 56, 241, 7, 7, ox = 1, oy = 6 },
    ['w'] = { 63, 241, 7, 7, ox = 1, oy = 6 },
    ['x'] = { 70, 241, 7, 7, ox = 1, oy = 6 },
    ['y'] = { 0, 232, 7, 9, ox = 1, oy = 6 },
    ['z'] = { 77, 241, 7, 7, ox = 1, oy = 6 },
    ['ú'] = { 0, 204, 7, 10, ox = 1, oy = 9 },
    ['|'] = { 75, 232, 3, 9, ox = -1, oy = 8 },
    ['ü'] = { 35, 232, 7, 9, ox = 1, oy = 8 },
    ['~'] = { 0, 248, 7, 4, ox = 1, oy = 6 },
    ['Ñ'] = { 0, 192, 7, 12, ox = 1, oy = 11 }
}

ui.mini_font = {}

local len = string.byte('z') - string.byte('a')
local rows = {
    { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k",  "l",    "m",    "n",     "o",      "p",     "q",     "r", "s",    "t", "u", "v", "w", "x", "y", "z" },
    { 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   "up", "left", "down", "right", "period", "comma", "shift", "?", "space" }
}

for y, row in ipairs(rows) do
    for x, element in ipairs(row) do
        ui.mini_font[tostring(element)] = {
            304 + (x - 1) * 6, 480 + (y - 1) * 6, 6, 6
        }
    end
end

local buttons = { "a", "b", "x", "y", "dpup", "dpleft", "dpdown", "dpright" }
for i, v in ipairs(buttons) do
    buttons[v] = { 320 + ((i - 1) * 16), 496, 16, 16 }
end


ui.font.fallback = ui.font["?"]

ui.rounded_rect = function(x, y, w, h, color)
    eng.rect(x + 1, y, w - 2, h, color)
    eng.rect(x, y + 1, w, h - 2, color)
end

ui.box = function(x, y, w, h, bg, fg)
    ui.rounded_rect(x, y, w, h, bg)
    eng.rect(x + 4, y + 4, w - 8, h - 8, fg)
    eng.rect(x + 5, y + 5, w - 10, h - 10, bg)
end

local NEWLINE_SPACE = 18
local LINE_SPACE = 12


ui.text_size = function(text)
    local px, py = 0, 0
    local sx, sy = 0, 6
    local whitespace_limit = 0

    for c in text:gmatch(utf8.charpattern) do
        local t = ui.font[c] or ui.font.fallback

        if (c == "\n") then
            px = 0
            py = py + NEWLINE_SPACE
            goto continue
        end

        sx = math.max(sx, px + t[3])
        sy = math.max(sy, py + t[4])

        px = px + 6

        if (c ~= " ") then
            whitespace_limit = sx
        end

        ::continue::
    end

    return sx, sy, whitespace_limit
end

local function print_naive(text, x, y, color)
    local nx, ny = x, y

    for c in text:gmatch(utf8.charpattern) do
        if c == "\n" then
            nx = x
            ny = ny + NEWLINE_SPACE
            goto continue
        end

        local t = ui.font[c] or ui.font.fallback

        eng.quad(t, nx - t.ox, ny - t.oy + 7, color)

        nx = nx + 6

        ::continue::
    end

    return nx
end

ui.print = function(text, x, y, color, wrap)
    if not wrap then
        return print_naive(text, x, y, color)
    end

    local nx, ny = x, y

    for line in text:gmatch("[^\r\n]+") do
        for word in line:gmatch("(%S+%s*)") do
            local w, _, ww = ui.text_size(word)

            if (nx + ww) > (x + wrap) then
                nx = x
                ny = ny + LINE_SPACE
            end

            print_naive(word, nx, ny, color)
            nx = nx + w
        end

        nx = x
        ny = ny + NEWLINE_SPACE
    end
end


-- TAKES WORLD SPACE, PIXEL SPACE (16 SUBDIVS PER METER)
local function print_naive_3d(text, vec, color)
    local v = vec3.add(vec, { 0, 9, -7 })

    local p = { 0, 0, 0 }

    for c in text:gmatch(utf8.charpattern) do
        if c == "\n" then
            p[2] = 0
            p[3] = p[3] - NEWLINE_SPACE
            goto continue
        end

        local t = ui.font[c] or ui.font.fallback

        local r = vec3.mul_val({
            v[1] + 0,
            v[2] + (p[2] - (t[3] - t.ox)),
            v[3] + (p[3] - (t[4] - t.oy))
        }, 1 / 16)

        eng.render {
            mesh = plane,
            model = mat4.from_transform(r, 0, { 1, t[3] / 16, t[4] / 16 }),
            texture = t,
            tint = color
        }

        p[2] = p[2] + 6

        ::continue::
    end
end

ui.print_3d = function(text, vec, color, wrap)
    --eng.render {
    --    mesh = assets.cube,
    --    model = mat4.from_transform(vec, 0, 0.1),
    --    texture = { 0, 0, 1, 1 }
    --}

    local v = vec3.mul_val(vec, 1 * 16)
    local p = { 0, 0, 0 }

    if not wrap then
        return print_naive_3d(text, p, color)
    end

    for line in text:gmatch("[^\r\n]+") do
        for word in line:gmatch("(%S+%s*)") do
            local w, _, ww = ui.text_size(word)

            if (p[2] + ww) > wrap then
                p[2] = 12
                p[3] = p[3] - 12
            end

            print_naive_3d(word, vec3.add(p, v), color)
            p[2] = p[2] + w
        end

        p[2] = 0
        p[3] = p[3] - 18
    end
end

ui.print_center = function(text, x, y, w, h, color)
    local tw, th = ui.text_size(text)

    print_naive(text, x + ((w - tw) // 2), y + ((h - th) // 2), color)
end


ui.squircle = function(x, y, w, h, color, radius, segments)
    local mesh = {
        data = "",
        length = 0
    }

    local function push(px, py)
        local ox = 0 --eng.perlin(px/600, py/600, eng.timer)   * 1.5
        local oy = 0 --eng.perlin(px/600, py/600, eng.timer/2) * 1.5

        mesh.length = mesh.length + 1
        mesh.data = mesh.data ..
            string.pack("<fffffBBBB", px + ox, py + oy, 0, 0, 0, 255, 255, 255, 255)
    end

    radius = 1 / fam.clamp(radius or 0.1, 0, 1)
    segments = segments or 30

    -- Center point of the squircle
    local cx, cy = x + w / 2, y + h / 2

    local points = {}

    for i = 0, segments do
        local angle = (i / segments) * (2 * math.pi)

        local cosAngle = math.cos(angle)
        local sinAngle = math.sin(angle)

        local factorX = (cosAngle > 0 and 1 or -1) * (math.abs(cosAngle) ^ (1 / radius))
        local factorY = (sinAngle > 0 and 1 or -1) * (math.abs(sinAngle) ^ (1 / radius))

        table.insert(points, {
            cx + factorX * w / 2,
            cy + factorY * h / 2
        })
    end

    -- Create triangles by fanning out from the center
    for i = 1, #points do
        local p1 = points[i]
        local p2 = points[i % #points + 1]
        push(p1[1], p1[2])
        push(p2[1], p2[2])
        push(cx, cy)
    end

    eng.draw {
        mesh = mesh,
        color = color
    }
end

ui.funky_rect = function(x, y, w, h, color)
    local mesh = {
        data = "",
        length = 0
    }

    local function push(px, py)
        local ox = eng.perlin(px / 300, py / 300, eng.timer / 4) * 3
        local oy = eng.perlin(px / 300, py / 300, eng.timer / 2) * 3

        mesh.length = mesh.length + 1
        mesh.data = mesh.data ..
            string.pack("<fffffBBBB", px + ox, py + oy, 0, 0, 0, 255, 255, 255, 255)
    end

    push(x, y)
    push(x + w, y)
    push(x, y + h)

    push(x + w, y)
    push(x, y + h)
    push(x + w, y + h)

    eng.draw {
        mesh = mesh,
        tint = color
    }
end

ui.funky_trongle = function(x1, y1, x2, y2, x3, y3, color)
    local mesh = {
        data = "",
        length = 0
    }

    local function push(px, py)
        local ox = eng.perlin(px / 300, py / 300, eng.timer / 4) * 3
        local oy = eng.perlin(px / 300, py / 300, eng.timer / 2) * 3

        mesh.length = mesh.length + 1
        mesh.data = mesh.data ..
            string.pack("<fffffBBBB", px + ox, py + oy, 0, 0, 0, 255, 255, 255, 255)
    end

    push(x1, y1)
    push(x2, y2)
    push(x3, y3)

    eng.draw {
        mesh = mesh,
        color = color
    }
end

ui.button = function(button, x, y, small)
    local type, name = button:lower():match("(.+):(.+)$")

    local s = small and 1 or 2

    if type == "keyboard" then
        --print(name)
        eng.quad({ 304, 496, 16, 16 }, x, y, 0xFFFFFFFF, s, s)

        local btn = ui.mini_font[name] or ui.mini_font["?"]
        eng.quad(btn, x + s * 3, y + s * 3, 0xFFFFFFFF, s, s)
    else
        eng.quad(buttons[name], x, y, 0xFFFFFFFF, s, s)
    end
end


ui.crap_menu = {}

ui.crap_menu.tick = function(self)
    if eng.input('up') == 1 then
        self.selected = self.selected - 1
        if self.selected < 1 then
            self.selected = #self
        end
    end

    if eng.input('down') == 1 then
        self.selected = self.selected + 1
        if self.selected > #self then
            self.selected = 1
        end
    end

    return eng.input("jump") == 1 and self.selected
end

ui.crap_menu.frame = function(self, x, y)
    for i, v in ipairs(self) do
        v = (self.selected == i and "→ " or "  ") .. v
        print_naive(v, x, y + (i - 1) * 10, 0xFFFFFFFF)
    end
end

ui.crap_menu.new = function(options)
    options.selected = options.selected or 1

    return setmetatable(options, { __index = ui.crap_menu })
end

ui.crap_menu.__call = function(self, ...) return ui.crap_menu.new(...) end

setmetatable(ui.crap_menu, ui.crap_menu)


return ui
