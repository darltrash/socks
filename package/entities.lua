local vec3 = require "lib.vec3"
local fam  = require "lib.fam"

local player = require "player"

local init = {
    player = player.init
}

local tick = {
    player = player.tick
}

return {
    init = init,
    tick = tick
}