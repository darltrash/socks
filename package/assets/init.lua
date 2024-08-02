local fam = require "lib.fam"
local mat4 = require "lib.mat4"

local assets = {}
local threads = {}
local loaded = {}

assets.mod_loading = eng.load_model("assets/mod_loading.exm")
assets.mus_wait = eng.load_sound("assets/mus_wait.ogg", false)

local dummy_mesh = {
    data = "",
    extra = "",
    submeshes = {},
    length = 0,
    loading = true
}
local function offload(name)
    local code = ("return eng.load_model('assets/%s.exm')"):format(name)
    local thread, err = eng.offload(code)

    print(thread, err)

    if thread then
        threads[name] = thread
    end

    if err then
        print("Asset thread error:", err)
    end

    return dummy_mesh
end

local function sound(name)
    return eng.load_sound("assets/" .. name .. ".ogg")
end

local loaders = {
    mod = offload,
    chr = offload,
    scr = offload,
    lvl = offload,
    mus = sound,
    snd = sound,
    amb = sound
}

assets.tick = function()
    local count = 0

    for name, thread in pairs(threads) do
        local data, err = eng.fetch(thread)

        if data then
            print("Asset loaded:", name)
            loaded[name] = data
            threads[name] = nil
        end

        if err then
            print("Asset thread error:", err)
            threads[name] = nil
        end

        if threads[name] then
            count = count + 1
        end
    end

    return count
end

assets.load_screen = function(which_files, callback)
    local time = 5
    local fade_out

    local s = eng.sound_play(assets.mus_wait, {
        gain = 0,
        looping = true,
    })

    eng.set_room {
        sprite = { 0, 0, 69, 8 },
        ticks = 0,

        init = function(self)
            eng.videomode(400, 300)
            for _, v in ipairs(which_files) do
                local t = assets[v]
            end
        end,

        tick = function(self)
            self.ticks = self.ticks - 1
            if self.ticks < 0 then
                self.sprite[1] = math.random(0, 143)
                self.sprite[2] = math.random(0, 63)
                self.ticks = 2
            end
        end,

        frame = function(self, alpha, delta)
            time = time - delta
            local t = assets.tick()

            if t == 0 and time < 0.0 and not fade_out then
                fade_out = 1
            end

            if fade_out then
                fade_out = fade_out - delta
                if fade_out < 0 then
                    fade_out = 0
                    eng.sound_stop(s)
                    callback()
                end
            end

            local a = (fade_out or 1) * fam.unlerp(5, 4, time)

            eng.sound_gain(s, a ^ 2)

            --eng.quad({ 48, 0, 32, 32 }, -200, -150, { 255, 255, 255, a })

            local mesh = {
                data = assets.mod_loading.data,
                length = math.floor(assets.mod_loading.length * (a / 3)) * 3
            }

            eng.draw {
                mesh = mesh,
                model = mat4.from_transform({ -200, 150, 0 }, 0, { 40, 40, 1 }),
                texture = self.sprite,
            }

            eng.draw {
                mesh = mesh,
                model = mat4.from_transform({ -200, 150, 0 }, 0, { 40, 40, 1 }),
                texture = { 0, 0, 1, 1 },
                tint = { 255, 255, 255, 40 }
            }
        end
    }
end

assets.loaded = function(name)
    return loaded[name]
end

setmetatable(assets, {
    __index = function(self, index)
        if not loaded[index] then
            local loader = loaders[index:sub(1, 3)]
            assert(loader)

            loaded[index] = loader(index)
        end

        return loaded[index]
    end
})

return assets
