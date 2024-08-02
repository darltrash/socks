local fam = require "lib.fam"

local w, h = 400, 300

return {
    i = 0,

    init = function()
        eng.videomode(w, h)
    end,

    tick = function(self)
        if eng.input("up") then
            self.i = 0
        end
    end,

    frame = function(self, alpha, delta)
        self.i = math.min(1, self.i + delta * 0.8)

        local i = fam.unlerp(0, 0.7, self.i)

        local w, h = eng.size()
        w = w + 2
        h = h + 2

        local k = 1 / 4

        local t1 = fam.unlerp(k * 0, (k * 0) + k, i) ^ 2
        local t2 = fam.unlerp(k * 1, (k * 1) + k, i) ^ 2
        local t3 = fam.unlerp(k * 2, (k * 2) + k, i) ^ 2
        local t4 = fam.unlerp(k * 3, (k * 3) + k, i) ^ 2

        local m = math.max(w, h) / fam.lerp(4, 2, fam.unlerp(0.7, 1.0, self.i) ^ 3)
        local wd = fam.lerp(20, m, (t1 + t2 + t3 + t4) / 4)

        eng.rect(-w / 2, -h / 2, w * t1, wd, { 255, 255, 255, fam.to_u8(t1) })

        eng.rect((w / 2) - wd, -h / 2, wd, h * t2, { 255, 255, 255, fam.to_u8(t2) })

        local nw = w * t3
        eng.rect((w / 2) - nw, (h / 2) - wd, nw, wd, { 255, 255, 255, fam.to_u8(t3) })

        local nh = h * t4
        eng.rect(-w / 2, (h / 2) - nh, wd, nh, { 255, 255, 255, fam.to_u8(t4) })
    end
}
