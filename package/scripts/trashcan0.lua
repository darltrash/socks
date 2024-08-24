return function(ent)
    ent.texture = { 80, 112, 32, 32 }
    ---@param s ScriptEnv
    ent.on_interact = function(s)
        s.say "A trash can full of green photosynthetizing stuff, kind of cool!"
        s.follow "It smells like what you think grass may smell like."
    end
end
