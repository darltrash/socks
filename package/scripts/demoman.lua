local fam = require "lib.fam"

local on_interact = function(s)
    s.actor "demo:happy"
    s.say "Hey! hello world!"
    s.actor "demo:neutral"
    s.say "I am the abstraction of a lovefar bygone hyper-structured creature between your frontal lobe and the artistic medium"
    s.actor "demo:happy"
    s.say "In other words..."
    s.actor "demo:glitch"
    s.say "I'm ____!"

    s.wait(2)
    s.actor "demo:neutral"
    s.say "Hm..."
    s.follow "Isn't this a bit boring? Let me put on some music!"
    local r = s.sfx("assets/snd_corny.ogg", true)

    local current = 1
    local lines = {
        "Banger, right?",
        "This part is great",
        "WAIT, Damn! Nice!",
        "I really like this part",
        "Amazing tbh",
        "I think this is epic",
        "Yeeeahh dude",
        "Mhm, epic",
        "Kind of amazing",
        "THAT CHORD BRO"
    }

    s.wait(1)
    s.actor "demo:happy"

    repeat
        s.say(lines[current % #lines + 1])
        current = current + 1
        s.wait(2)
    until r.done

    s.actor "demo:neutral"
    s.say "..."
    s.follow "that sucked..."

    s.actor "demo:happy"
    s.say "BUT HEY, LOOK AT THIS!"
    s.image "assets/scr_cami_evilsoda.exm"
    s.follow "cool, right?"

    s.actor "demo:glitch"
    s.say "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

    s.actor "demo:neutral"
    s.say "ok that was it bye"
    os.exit()
end

return function(ent)
    ent.texture = { 464, 480, 32, 32 }
    ent.on_interact = on_interact
end
