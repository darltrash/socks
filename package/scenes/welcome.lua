local ui = require "ui"

local precursor = 0
local cursor = 0
local text = ""
local busy = false
local options = false
local selection = 1
local light_mode = false
--

local tick = eng.load_sound("assets/snd_tick.ogg")

local function say(msg)
    cursor = 0
    text = msg
    busy = true
    while busy do
        coroutine.yield()
    end
end

local function ask(msg, answers)
    options = answers
    selection = 1
    say(msg)

    return selection
end

local function speech()
    light_mode = ask("Can you read this properly?", { "Yes", "No" }) == 2
    if light_mode then
        light_mode = ask("What about now?", { "Better.", "Revert it." }) == 1
    end

    say "So, I've made a little something for you to experience, it's something that I've been meaning to show you for a while..."
    say "It's very important to me, so, if you could give me feedback about it, I will be available on my Signal or PM through Fedi."
    local photosensitive = ask("As far as I am aware, you're not photosensitive, are you?", { "I am.", "I am not." }) ==
    1

    if photosensitive then
        say "Then...\nI am sorry, I am not sure I want you to play this game, I will close the game now."
        say "Goodbye"
        os.exit()
        return
    end

    say "Then...\nLet's begin!"

    say "Goodnight, Sleepyhead!"
end

return {
    routine = coroutine.create(speech),

    init = function(self)
        eng.videomode(300, 200)

        precursor = 0
        cursor = 0
        text = ""
        busy = false
        options = false
        selection = 1
        light_mode = false
    end,

    tick = function(self)
        local v = 0.53
        local c = math.floor(cursor)
        local k = text:sub(c, c)

        if k == "," then
            v = 0.13
        elseif k == "." then
            v = 0.07
        end

        if eng.input("attack") then
            v = 0.8
        end

        cursor = math.min(cursor + v, #text)

        if precursor ~= math.floor(cursor) then
            eng.sound_play(tick)
        end
        precursor = math.floor(cursor)

        if cursor == #text then
            if eng.input("jump") then
                busy = false
                text = ""
                cursor = 0
                options = false
            end


            if options then
                if eng.input("up") == 1 then
                    selection = selection - 1
                    if selection == 0 then
                        selection = #options
                    end
                end

                if eng.input("down") == 1 then
                    selection = selection + 1
                    if selection > #options then
                        selection = 1
                    end
                end
            end
        end


        coroutine.resume(self.routine)
    end,

    -- 2:40 pm

    frame = function(self)
        local color_a = light_mode and 0xFFFFEBFF or 0x101010FF
        local color_b = light_mode and 0x101010FF or 0xFFFFEBFF

        eng.far(1, color_a)

        local c = text:sub(1, math.floor(cursor))
        --ui.print(c, -120, -29, 0x000000FF, 240)
        ui.print(c, -120, -30, color_b, 240)

        if options and cursor == #text then
            for i, option in ipairs(options) do
                if selection == i then
                    eng.rect(-124, (30 + ((i - 1) * 20)) - 5, 100, 16, color_b)
                    ui.print(option, -120, 30 + ((i - 1) * 20), color_a)
                else
                    ui.print(option, -120, 30 + ((i - 1) * 20), color_b)
                end
            end
        end
    end
}
