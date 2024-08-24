local mat4 = require "lib.mat4"
local fam = require "lib.fam"

local screen = {}

screen.init = function(self, which, callback)
    eng.videomode(400, 300)
    self.model = eng.load_model(which)
    self.transition = -1
    self.lock = true
    self.callback = callback
    self.unlock_timer = 0
    self.unlock_a = 0
end

screen.frame = function(self, alpha, delta)
    self.transition = self.transition + delta
    if self.lock and self.transition >= 0 then
        self.unlock_timer = eng.input("jump") or 0

        self.unlock_a = fam.lerp(self.unlock_a, (self.unlock_timer / 30) + 0.1, delta * 2)
        if self.unlock_a > 1 then
            self.unlock_a = 1
            self.lock = false
        end

        self.transition = 0
    end

    if self.transition >= 1 then
        self.callback()
        self.transition = 1
        self.model = nil
    end

    local a = 1.0 - math.abs(self.transition)
    local b = (1 - a) ^ 2

    if not self.model then
        return
    end

    eng.draw {
        mesh = self.model,
        model = mat4.from_transform(
            { -200 - (b * 120), -150 + (b * 100), 2 },
            { 0, 0, (1 - a) ^ 3 },
            { 100, 100, 0.01 }
        ),
        texture = { 0, 0, 1, 1 },
        tint = { 255, 255, 255, fam.to_u8(a ^ 3) }
    }

    local ca = {
        0x00, 0x00, 0x00, fam.to_u8(a ^ 3)
    }
    eng.rect(-200 + 5, -150 + 5, 20, 20, ca)

    local cb = {
        0xFF, 0xFF, 0xFF, ca[4]
    }
    eng.rect(-200 + 6, -150 + 6, 18, (self.unlock_a ^ 3) * 18, cb)
end

return screen
