return function(ent)
    ent.invisible = true

    ent.on_interact = function(s)
        s.say "A beautiful piece of abstract art"
        s.follow "Or an illegible community notice, perhaps...?"
        s.say "I still don't understand what the purpose of the scribble and bars is"
    end
end
