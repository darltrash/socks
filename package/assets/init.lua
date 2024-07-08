return {
    plane = eng.load_model("assets/mod_plane.exm"),
    cube = eng.load_model("assets/mod_cube.exm"),
    shadow = eng.load_model("assets/mod_shadow.exm"),
    fan = eng.load_model("assets/mod_fan.exm"),
    switch = eng.load_model("assets/mod_switch.exm"),
    lock = eng.load_model("assets/mod_lock.exm"),
    sphere = eng.load_model("assets/mod_sphere.exm"),

    the_funny = eng.load_sound("assets/snd_boom.ogg"),
    jump = eng.load_sound("assets/snd_jump.ogg"),
    grab = eng.load_sound("assets/snd_grab.ogg"),
    pop  = eng.load_sound("assets/snd_pop.ogg"),
    floor_hit = eng.load_sound("assets/snd_floor_hit.ogg"),
    switch_click = eng.load_sound("assets/snd_switch.ogg"),
    transition_in = eng.load_sound("assets/snd_transition_in.ogg"),
    transition_out = eng.load_sound("assets/snd_transition_out.ogg"),
    --crocodile_music_for_their_non_existant_ears = eng.load_sound("assets/mus_crocodile.ogg")

    --wereflying = eng.load_sound("assets/mus_wereflying.ogg", true),

    city = eng.load_sound("assets/amb_city.ogg")
}