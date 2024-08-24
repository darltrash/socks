local language = require "language"

return function(ent)
    ent.texture = { 464, 480, 32, 32 }
    ent.tint = { 255, 255, 255, 255 }

    ---@param s ScriptEnv
    ent.on_interact = function(s)
        s.actor "demo:happy"
        s.say "Hello, I'm DEMO!"

        s.actor "demo:loading"
        s.follow "Here's a bunch of randomly generated nonsense"

        s.actor "demo:happy"
        s.say(language.markov("I", 25))

        s.actor "default"
        s.say "goodnight."
    end
end
