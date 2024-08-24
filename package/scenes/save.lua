local ui = require "ui"
local save = require "save"

local selection = 1

local mode
local text

return {
    init = function()
        eng.videomode(400, 300)
    end,

    tick = function()
        if eng.input("up") == 1 then
            selection = selection - 1
        end

        if eng.input("down") == 1 then
            selection = selection + 1
        end

        if mode == "function" then
            text = (text or "") .. eng.text()
            if text:find("%(") then
                mode = nil
            end
        end
    end,

    frame = function()
        --local t = os.time() - save.data.SV_CLOCK
        --ui.print(("%i:%01i"):format(math.floor(t / 60), t % 60), -120 + 10, -100)

        if mode == "function" then
            return
        end

        local i = 0
        local y = -100 + 10 + (selection * -9)

        local iterate
        iterate = function(t, tab)
            for name, value in pairs(t) do
                i = i + 1

                local ty = type(value)
                local g = (selection == i and "→ " or "  ") .. (" "):rep(tab)

                if ty == "table" then
                    ui.print(g .. name .. ":", -100 + 10, y)
                    y = y + 10
                    iterate(value, tab + 1)
                    y = y + 10
                else
                    --if eng.input("attack") then
                    --    if ty == "function" then
                    --        __func = value
                    --        load("__func()")()
                    --    end
                    --end

                    local str = ("%s%-20s %s"):format(g, name, value)
                    ui.print(str, -100 + 10, y)
                    y = y + 10
                end
            end
        end

        if i > 0 then
            selection = (selection % i) + 1
        end


        iterate(eng, 0)
    end
}
