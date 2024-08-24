return function(ent)
    ent.invisible = true
    ent.on_interact = function(s)
        s.say "For some reason, this wet floor sign says \"RUN\"..."
        s.follow "That sounds like the opposite thing you'd want for a wet floor sign"

        s.say "Maybe it's here because the floor isn't wet."

        ent.on_interact = function(s)
            s.say "Oh, wait, nevermind, it says \"FUN\""
            s.follow "To my defense, it DOES have a little guy running drawn on it"

            ent.on_interact = function(s)
                s.say "I feel a bit stupid..."
            end
        end
    end
end
