return function(ent)
    ent.texture = { 256, 32, 32, 48 }
    ---@param s ScriptEnv
    ent.on_interact = function(s)
        s.say "Nice night, isn't it? Just the right moment for a picnic"
        s.follow "... If I had the actual stuff you put at a picnic"
        s.say "Eh, Life's nice still nonetheless perhaps, isn't it?"

        ent.on_interact = function()
            s.say "Yeah!"
        end
    end
end
