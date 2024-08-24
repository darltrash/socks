local binser = require "lib.binser"

-- SEXO PORNO!!!
eng.identity("socks")

local save = {
    data = {}
}

save.load = function()
    local e = eng.retrieve()
    if not e then
        save.data.SV_CLOCK = os.time()
        save.write()
        return false
    end

    local ok, res = pcall(binser.deserialize, e)

    assert(ok, "ERROR LOADING SAVEFILE, YOU BROKE SOMETHING.")
    save.data = res[1]

    return true
end

save.write = function()
    eng.store(binser.serialize(save.data))
end

save.load()

return save
