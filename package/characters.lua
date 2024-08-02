local chars = {}

chars.default = {
    avatar = nil,
    name = nil,

    bg = { 0xEE, 0xEE, 0xEE, 0xFF },

    speech = {
        eng.load_sound("assets/snd_tick.ogg")
    }
}

chars.demo = {
    avatar = eng.load_model("assets/chr_demoman.exm"),
    name = "demo",

    bg = { 0xee, 0xaa, 0xff, 255 },

    speech = {
        eng.load_sound("assets/snd_speech_demoman0.ogg"),
        eng.load_sound("assets/snd_speech_demoman1.ogg"),
        eng.load_sound("assets/snd_speech_demoman2.ogg"),
    }
}

-- panda girl, weedz
chars.lyu = {
    avatar = eng.load_model("assets/chr_lyu.exm"),
    name = "Lyu",
    is_lesbian = true,

    bg = { 0xd4, 0xbe, 0xfa, 255 },

    --[[
        https://www.youtube.com/watch?v=SSBWiFGzsyU
    ]]

    speech = {
        eng.load_sound("assets/snd_speech_lyu.ogg")
    },

    sfx = {
        shock = eng.load_sound("assets/snd_shock.ogg")
    }
}

return chars
