return function(ent)
    ent.invisible = true

    ent.on_interact = function(s)
        s.say "It's supposed to be a swing but..."
        s.follow "You can't reach the thingy for sitting in"
        s.follow "And it's completely stiff!"
        s.say "To be fair, if this swing actually worked, we would see much more cases of children flinging themselves through the air"
    end
end
