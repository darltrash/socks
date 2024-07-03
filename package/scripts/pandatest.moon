import say, ask, profile, clear, display from require "dialog"
import sound from require "scripts"

=>
    profile "lyu.shock"
    say "DAMN, BOY!"
    say "ya THICC!"
    clear!

    profile "lyu.uncomfortable"
    if ask("is this normal to say?", {"yes", "no"}) == 1
        profile "lyu.neutral"
        sound "snd_orchhit.ogg"
        say "cool as feck, boi."
    else
        profile "lyu.sad"
        sound "snd_wompwomp.ogg"
        say "damn..."
    clear!

    display "YOU GOT: ABSOLUTELY NOTHING!"

    shit yourself!