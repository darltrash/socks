#!/usr/bin/env luajit

local function exec(...)
    local t = table.concat({...}, " ")
    print(t)
    local a = assert(io.popen(t))
    local t = a:read("*a")
    a:close()

    return t
end

local function check_exec(name)
    local a = exec("whereis " .. name)
    if #a == (#name+2) then
        error("program '"..name.."' wasnt found in $PATH! go get it checked and installed :/\n", 2)
    end
end

local function opt_exec(name)
    local a = exec("whereis " .. name)
    return #a ~= (#name+2)
end

if package.config:sub(1,1) ~= "/" then
    print("note!! this program wasnt made with windows on mind, so expect it to break.\n")
end

check_exec("find")
os.execute("mkdir -p out")

local commit = "unknown"
if opt_exec("git") then
    commit = exec("git rev-parse --short HEAD")
    commit = commit:gsub("%s", "")
end

local packed_stuff = false

local pack = function ()
    if packed_stuff then return end

    os.execute [[
        cd package;
        zip ../package.bsk -9 -r *
    ]]
end

local compiled_moon = false

local function moon()
    if compiled_moon then return end
    compiled_moon = true

    if not opt_exec("moonc") then
        print("Could not find moonc in $PATH, cannot compile .moon files.")
        return
    end

    for n in io.popen("find package/ -type f -name \"*.moon\""):lines() do
        exec("moonc", n)
        print()
    end
end

local default_toolchain = {
    name = "unknown",
    
    cc = "gcc",
    flags = "-lm -lSDL2",
    strip = "strip",
    post = false,
    extension = "",

    should_strip = true,
    should_compress = true,

    debug = false
}

local function compile(setup)
    moon()

    if not setup.debug then
        pack()
    end

    setmetatable(setup, { __index = default_toolchain })

    local t = setup.debug and "debug" or "release"
    print("\nbuilding using the '"..setup.name.."' ("..t..") target")

    check_exec(setup.cc)

    if opt_exec("ccache") and not os.getenv("NO_CCACHE") then
        print("! found ccache and env var NO_CCACHE is undefined")
        setup.cc = "ccache " .. setup.cc
    end

    local k = "-Ilib/ -Wstringop-overflow=0 -D_POSIX_C_SOURCE=200809L --std=c99 -DBASKET_COMMIT='\""..commit.."\"' "
        .. (setup.file_flags or "")
    
    local optimizations = "-fdata-sections -ffunction-sections -Wl,--gc-sections"
    if setup.debug then
        k = k .. " -ggdb -DTFX_DEBUG -DTRASH_DEBUG"
        optimizations = "-ggdb"
        print("IGNORING target.should_strip AND target.should_compress")
    else
        k = k .. " -Ofast"
    end

    local dir = "out/"..setup.name.."/"
    os.execute("mkdir -p " .. dir)
    for n in io.popen("find basket/ -name \"*.c\" -type f"):lines() do
        local o_name = dir..n:sub(1, #n-2):gsub("/", ".")..".o"
        local c = ("%s %s -c %s -o %s"):format(setup.cc, k, n, o_name)
        print("> compiling '"..n.."'")
        exec(c)
        print()
    end

    local output = "thing" .. setup.extension


    print("> linking")
    exec(setup.cc, optimizations, dir.."*.o", setup.flags, "-o", output)
    print()

    local function attempt(what)
        if not what then
            return
        end

        io.stdout:write("> trying to use " .. what .. " on output")
        if setup.debug then
            return io.stdout:write(" ... debug mode, skipping\n")
        end

        if not opt_exec(what) then
            return io.stdout:write("... not found!\n")
        end

        exec(what, output)
    
        io.stdout:write(" ... ok!\n")
    end

    attempt(setup.strip)

    if setup.post then
        print("> post")
        print(setup.post)
        os.execute(setup.post:gsub("\n", ";\n"))
    end

    print("!! done !!")
end

local function release_linux(dbg)
    compile {
        name = "x86_64-linux-host",
    
        cc = os.getenv("CC") or "gcc",
        strip = "strip",
        flags = "-lm -lSDL2 -lc -ldl",

        debug = dbg,

        post = [[
            rm -rf out/socks-lin64
            mkdir -p out/socks-lin64
            cp thing       out/socks-lin64/socks
            cp package.bsk out/socks-lin64
            cd out/socks-lin64
            zip ../socks-lin64.zip socks package.bsk
        ]]
    }
end

local function release_windows(dbg)
    check_exec("zip")
    
    compile {
        name = "x86_64-windows-gnu",

        cc = "x86_64-w64-mingw32-gcc",
        flags = "-lSDL2",
        strip = "x86_64-w64-mingw32-strip",
        extension = ".exe",
        
        debug = dbg,

        post = [[
            rm -rf out/socks-win64
            mkdir -p out/socks-win64
            cp thing.exe               out/socks-win64/socks.exe
            cp package.bsk             out/socks-win64
            cp basket/lib/bin/SDL2.dll out/socks-win64
            cd out/socks-win64
            zip ../socks-win64.zip socks.exe SDL2.dll package.bsk
        ]]
    }
end

local function release_wasm()
    if true then
        print("Unsupported :(")
        return
    end

    compile {
        name = "wasm",

        cc = "emcc",
        flags = "-sUSE_SDL=2 -sOFFSCREEN_FRAMEBUFFER=1 -sFULL_ES3=1 -sWASM=1",
        file_flags = "-DSDL_DISABLE_IMMINTRIN_H=1 -DCUTE_SOUND_SCALAR_MODE=1",
        extension = ".js",
        strip = false,
        upx = false, -- not even supported

        post = [[
            rm -rf out/socks-html5
            mkdir -p out/socks-html5
            cp thing.js out/socks-html5
            cp thing.wasm out/socks-html5
            cp raw/index.html out/socks-html5
        ]]
    }
end

local function release(dbg)
    release_linux(dbg)
    release_windows(dbg)

    if not dbg then
        release_wasm()
    end
end

local function shader()
    -- shittier version of pack(), specific for shaders

    local out = assert(io.open("basket/shaders.h", "w"))

    out:write "// ./build.lua shader\n\n"

    print()

    for n in io.popen("find basket/shaders/* -type f"):lines() do
        io.stdout:write("> Encoding '", n, "' ... ")

        out:write ("// " .. n .. "\n")
        out:write "const char "
        local g = n:match("^.+/(.+)$"):gsub("%W", "_")
        out:write(g)
        out:write "[] = {\n\t"

        local bytecount = 0
        local f = assert(io.open(n, "rb"))
        while f:read(0) do
            out:write(("0x%02X, "):format(f:read(1):byte()))
            bytecount = bytecount + 1
            if bytecount > 9 then
                bytecount = 0
                out:write "\n\t"
            end
        end

        out:write "0x00\n};\n\n"

        io.stdout:write("OK\n")
    end
    io.stdout:write("\nDone.\n")

    out:close()
end

local function debug()
    release(true)
end

local function debug_linux()
    release_linux(true)
end

local function debug_windows()
    release_windows(true)
end

-- I am sorry, windows users
local function run()
    print("\n!! fast run mode!!! aeeee !!")

    moon()

    check_exec("tcc")

    local a = "-Ilib/ -lSDL2 -lm -pthread"
    local k = "-DTRASH_DEBUG -DFS_NAIVE_FILES -DSDL_DISABLE_IMMINTRIN_H -DCUTE_SOUND_SCALAR_MODE -DSTBI_NO_SIMD"

    local c = "tcc "..a.." "..k.." basket/*.c basket/lib/*.c -o thing"
    os.execute(c)
    print(c)
    os.execute("./thing")

    print("all is okay! :)")
end

local function cleanup()
    print("\n!! cleaning up this ugly garbage !!")

    os.execute("rm -f basket/static.h")
    os.execute("rm -f thing thing.*")
    os.execute("rm -rf package.bsk")
    os.execute("rm -rf out")
end

print("hey i build stuff hello")

local options = {
    cleanup = cleanup,
    pack = pack,
    run = run,
    moon = moon,

    release = release,
    release_linux = release_linux,
    release_windows = release_windows,

    debug = debug,
    debug_linux = debug_linux,
    debug_windows = debug_windows,

    release_wasm = release_wasm,

    shader = shader
}

options.help = function ()
    local g = {}
    for name in pairs(options) do
        table.insert(g, name)
    end

    print("\nthis is what i can do:")
    print("\t" .. table.concat(g, "\n\t"))
    os.exit(0)
end

local opt = arg[1]
if not opt then
    options.help()
end

local f = options[opt]
if not f then
    print("darn, idk what '"..opt.."' is")
    options.help()
end

f()