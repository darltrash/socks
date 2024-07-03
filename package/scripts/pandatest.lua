local say, ask, profile, clear, display
do
  local _obj_0 = require("dialog")
  say, ask, profile, clear, display = _obj_0.say, _obj_0.ask, _obj_0.profile, _obj_0.clear, _obj_0.display
end
local sound
sound = require("scripts").sound
return function(self)
  profile("lyu.shock")
  say("DAMN, BOY!")
  say("ya THICC!")
  clear()
  profile("lyu.uncomfortable")
  if ask("is this normal to say?", {
    "yes",
    "no"
  }) == 1 then
    profile("lyu.neutral")
    sound("snd_orchhit.ogg")
    say("cool as feck, boi.")
  else
    profile("lyu.sad")
    sound("snd_wompwomp.ogg")
    say("damn...")
  end
  clear()
  display("YOU GOT: ABSOLUTELY NOTHING!")
  return shit(yourself())
end
