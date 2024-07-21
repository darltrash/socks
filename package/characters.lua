local chars = {}

chars.demo = {
    avatar = eng.load_model("assets/chr_demoman.exm"),
    name = "demo",

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
