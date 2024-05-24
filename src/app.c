// implements the lua api that pilots this huge mech of a tech stack.

#include <stddef.h>
#include <stdio.h>
#define STB_VORBIS_INCLUDE_STB_VORBIS_H 1

#include "common.h"
#include "minilua.h"
#include "cute_sound.h"
#include "stb_perlin.h"

static lua_State *l;

static void get_vector(f32 *out, int tableIndex, int len) {
    if (!lua_istable(l, tableIndex)) {
        luaL_error(l, "Argument must be a table");
        return;
    }

    for (size_t i = 0; i < len; ++i) {
        lua_pushnumber(l, i+1);
        lua_gettable(l, tableIndex);

        if (!lua_isnumber(l, -1)) {
            luaL_error(l, "Field '%i' is not a number", i);
            return;
        }

        out[i] = lua_tonumber(l, -1);

        lua_pop(l, 1);
    }
}

// eng.read(filename) -> string?
static int api_fs_read() {
    u32 size;
    const char *str = fs_read(luaL_checkstring(l, 1), &size);

    if (str == NULL) 
        lua_pushnil(l);
        
    else
        lua_pushlstring(l, str, size);

    return 1;
}

// eng.camera(from, to)
static int api_camera() {
    f32 a[3];
    f32 b[3];
    get_vector(a, 1, 3);
    get_vector(b, 2, 3);

    ren_camera(a, b);

    return 0;
}

// eng.load_model(filename) -> { begin, length }
static int api_load_model() {
    const char *file = luaL_checkstring(l, 1);

    u32 size;
    const char *str = fs_read(file, &size);

    if (str == NULL) {
        luaL_error(l, "file doesnt exist, this one -> \"%s\"", file);
        return 0;
    }
        
    Map model = mod_load(str);

    // TODO: Transform to a special metatype
    lua_newtable(l);
    lua_pushlstring(l, (void*)model.mesh.data, model.mesh.length * sizeof(Vertex));
    lua_setfield(l, -2, "data");
    lua_pushinteger(l, model.mesh.length);
    lua_setfield(l, -2, "length");
    if (model.extra) {
        lua_pushstring(l, model.extra);
        lua_setfield(l, -2, "extra");
    }

    free(model.mesh.data);

    return 1;
}

static int source_amount = 0;
static cs_audio_source_t *sources[128];

static int api_load_sound() {
    const char *file = luaL_checkstring(l, 1);

    u32 size;
    const char *str = fs_read(file, &size);

    if (str == NULL) {
        luaL_error(l, "file doesnt exist, this one -> \"%s\"", file);
        return 0;
    }
    
    cs_error_t err;
    cs_audio_source_t *source = cs_read_mem_ogg(str, size, &err);
    if (source == NULL) {
        luaL_error(l, "error loading source \"%s\" (%s)", file, cs_error_as_string(err));
        return 0;
    }

    lua_pushnumber(l, source_amount);

    sources[source_amount++] = source;

    return 1;
}

static Color fetch_color(int a) {
    Color color = COLOR_WHITE;

    if (!lua_isnoneornil(l, a)) {
        // 0xFFFFFFFF
        if (lua_isinteger(l, a))
            color.full = (u32)lua_tointeger(l, a);

        // { 0xFF, 0xFF, 0xFF, 0xFF }
        else if (lua_istable(l, a)) {
            for (int i = 1; i <= 4; ++i) {
                lua_rawgeti(l, a, i);
                color.array[3 - (i - 1)] = clamp(luaL_checknumber(l, -1), 0, 255);

                lua_pop(l, 1);
            }
            
        } else
            luaL_error(l, "tint value expected to be integer, nil or table of 4 integers");

    }

    return color;
}

static TextureSlice fetch_texture(int a) {
    TextureSlice texture = { 0, 0, 0, 0 };

    if (!lua_isnil(l, a)) {
        if (!lua_istable(l, a)) {
            luaL_error(l, "Argument must be a table");
            return texture;
        }

        static u16 tmp[4];

        for (int i = 1; i <= 4; ++i) {
            lua_rawgeti(l, a, i);
            luaL_checkinteger(l, -1);
            tmp[i - 1] = lua_tointeger(l, -1);
            lua_pop(l, 1);
        }

        texture.x = tmp[0];
        texture.y = tmp[1];
        texture.w = tmp[2];
        texture.h = tmp[3];
    }

    return texture;
}

// eng.render(render_call)
static int api_render() {
    if (!lua_istable(l, 1)) {
        luaL_error(l, "Argument must be a table");
        return 0;
    }

    RenderCall call = { false, IDENTITY_MATRIX, COLOR_WHITE, {}, {0, 0, 0, 0} };

    lua_getfield(l, 1, "disable");
    if (!lua_isnil(l, -1))
        call.disable = lua_toboolean(l, -1);

    lua_getfield(l, 1, "model");
    if (!lua_isnil(l, -1)) {
        if (!lua_istable(l, -1)) {
            luaL_error(l, "'model' property must be a table");
            return 0;
        }

        for (int i=1; i <= 16; i++) {
            lua_pushinteger(l, i);
            lua_gettable(l, -2);
            call.model[i-1] = (f32)lua_tonumber(l, -1);

            lua_pop(l, 1);
        }
    }

    lua_getfield(l, 1, "tint");
    call.tint = fetch_color(-1);

    lua_getfield(l, 1, "mesh");
    if (!lua_istable(l, -1)) {
        luaL_error(l, "'mesh' property must be a table");
        return 0;
    }

        lua_getfield(l, -1, "data");
        call.mesh.data = (void*)lua_tostring(l, -1);
        lua_pop(l, 1);
        
        lua_getfield(l, -1, "length");
        call.mesh.length = lua_tointeger(l, -1);
        lua_pop(l, 1);

    lua_getfield(l, 1, "texture");
    call.texture = fetch_texture(-1);

    ren_draw(call);

    return 0;
}

// eng.light(pos, color)
static int api_light() {
    Light light;

    luaL_checktype(l, 1, LUA_TTABLE);
    for (int i = 0; i < 3; ++i) {
        lua_rawgeti(l, 1, i + 1);
        luaL_checknumber(l, -1);
        light.position[i] = lua_tonumber(l, -1);
        lua_pop(l, 1);
    }

    luaL_checktype(l, 2, LUA_TTABLE);
    for (int i = 0; i < 3; ++i) {
        lua_rawgeti(l, 2, i + 1);
        luaL_checknumber(l, -1);
        light.color[i] = lua_tonumber(l, -1);
        lua_pop(l, 1);
    }

    ren_light(light);

    return 0;
}


static int api_far() {
    ren_far(luaL_checknumber(l, 1), fetch_color(2));
    return 0;
}

static int api_dithering() {
    ren_dithering(lua_toboolean(l, 1));
    return 0;
}

static int api_ambient() {
    ren_ambient(fetch_color(1));
    return 0;
}

static int api_snapping() {
    ren_snapping(luaL_checknumber(l, 1));
    return 0;
}



// ren.quad(texture, x, y, color)
static int api_quad() {
    Quad quad = DEFAULT_QUAD;

    quad.texture = fetch_texture(1);
    quad.position[0] = lua_tonumber(l, 2);
    quad.position[1] = lua_tonumber(l, 3);
    quad.color = fetch_color(4);

    ren_quad(quad);

    return 0;
}

// ren.rect(x, y, w, h, color)
static int api_rect() {
    ren_rect(
        lua_tonumber(l, 1), 
        lua_tonumber(l, 2), 
        lua_tonumber(l, 3), 
        lua_tonumber(l, 4), 
        fetch_color(5)
    );

    return 0;
}

static int api_input() {
    // KEEP THIS SYNCED WITH COMMON.H
    const char *options[] = {
        "none",
        "up",
        "down",
        "left",
        "right",
        "jump",
        "attack",
        "menu",
        NULL
    };

    int e = luaL_checkoption(l, 1, NULL, options);
    int o = inp_button(e);

    if (o) 
        lua_pushinteger(l, o);
    else
        lua_pushnil(l);

    return 1;
}

static int api_text() {
    lua_pushstring(l, inp_text());
    return 1;
}

static int api_log() {
    size_t _ = 0;
    ren_log("%s", luaL_tolstring(l, 1, &_));

    return 0;
}

static int api_mouse_down() {
    bool down = eng_mouse_down(luaL_checkinteger(l, 1));
    lua_pushboolean(l, down);

    return 1;
}

static int api_mouse_position() {
    u16 x, y;
    eng_mouse_position(&x, &y);

    lua_pushinteger(l, x);
    lua_pushinteger(l, y);

    return 2;
}

static int api_window_size() {
    u16 w, h;
    eng_window_size(&w, &h);

    lua_pushinteger(l, w);
    lua_pushinteger(l, h);

    return 2;
}

static int api_size() {
    u16 w, h;
    ren_size(&w, &h);

    lua_pushinteger(l, w);
    lua_pushinteger(l, h);

    return 2;
}


// eng.music_volume(volume)
static int api_music_volume() {
    cs_music_set_volume(lua_tonumber(l, 1));
    return 0;
}

// eng.music_loop(loopś)
static int api_music_loop() {
    cs_music_set_loop(lua_toboolean(l, 1));
    return 0;
}

// eng.music_crossfade(source, fade)
static int api_music_crossfade() {
    cs_audio_source_t *source = sources[lua_tointeger(l, 1)];
    cs_music_crossfade(source, lua_tonumber(l, 2));
    return 0;
}


// eng.music_switch(source, fade-in, fade-out)
static int api_music_switch() {
    cs_audio_source_t *source = sources[lua_tointeger(l, 1)];
    cs_music_switch_to(source, lua_tonumber(l, 2), lua_tonumber(l, 3));
    return 0;
}

// eng.music_play(source, fade-in)
static int api_music_play() {
    cs_audio_source_t *source = sources[lua_tointeger(l, 1)];
    cs_music_play(source, lua_tonumber(l, 2));
    return 0;
}

static int api_music_stop() {
    f32 time = 0.0;
    if (!lua_isnoneornil(l, 1))
        time = lua_tonumber(l, 1);
        
    cs_music_stop(time);
    return 0;
}

// eng.sound_play(source, params) -> sound
static int api_sound_play() {
    cs_audio_source_t *source = sources[lua_tointeger(l, 1)];

    cs_sound_params_t params = cs_sound_params_default();
    if (!lua_isnoneornil(l, 2)) {
        luaL_checktype(l, 2, LUA_TTABLE);

        lua_getfield(l, 2, "paused");
        if (!lua_isnil(l, -1))
            params.paused = lua_toboolean(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, 2, "looped");
        if (!lua_isnil(l, -1))
            params.looped = lua_toboolean(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, 2, "volume");
        if (!lua_isnil(l, -1))
            params.volume = luaL_checknumber(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, 2, "pan");
        if (!lua_isnil(l, -1))
            params.pan = luaL_checknumber(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, 2, "delay");
        if (!lua_isnoneornil(l, -1))
            params.delay = luaL_checknumber(l, -1);
        lua_pop(l, 1);
    }

    cs_playing_sound_t sound = cs_play_sound(source, params);

    lua_pushinteger(l, sound.id);
    return 1;
}

static int api_sound_volume() {
    cs_playing_sound_t sound = { luaL_checkinteger(l, 1) };

    if (lua_isnoneornil(l, 2)) {
        lua_pushnumber(l, cs_sound_get_volume(sound));
        return 1;
    }

    f32 volume = lua_tonumber(l, 2);
    cs_sound_set_volume(sound, volume);
    lua_pushnumber(l, volume);
    
    return 1;
}

// eng.sound_loop(source, pan)
static int api_sound_pan() {
    cs_playing_sound_t sound = { luaL_checkinteger(l, 1) };
    cs_sound_set_pan(sound, lua_tonumber(l, 2));
    return 0;
}

static int api_sound_is_playing() {
    cs_playing_sound_t sound = { luaL_checkinteger(l, 1) };
    lua_pushboolean(l, cs_sound_is_active(sound));
    return 1;
}

static int api_perlin() {
    float x = luaL_checknumber(l, 1);
    float y = luaL_checknumber(l, 2);
    float z = luaL_checknumber(l, 3);

    int seed = 0x1337C0DE;
    if (!lua_isnoneornil(l, 4))
        seed = luaL_checkinteger(l, 4);

    lua_pushnumber(l, stb_perlin_noise3_seed(x, y, z, 0, 0, 0, seed));
    return 1;
}

static int api_identity() {
    int o = sav_identity(lua_tostring(l, 1));
    if (o)
        luaL_error(l, "Could not set savefile identity!");

    return 0;
}

static int api_store() {
    size_t size;
    const char *str = luaL_checklstring(l, 1, &size);
    int o = sav_store(str, size);
    if (o)
        luaL_error(l, "Could not store into savefile!");

    return 0;
}

static int api_retrieve() {
    u32 size = 0;
    char *a = sav_retrieve(&size);
    
    if (size == 0) {
        lua_pushboolean(l, false);
        return 1;
    }

    lua_pushlstring(l, a, size);
    free(a);
    return 1;
}

static int api_exit() {
    eng_close();
    return 0;
}


static const luaL_Reg registry[] = {
    { "read",       api_fs_read    },
    { "camera",     api_camera     },
    { "load_model", api_load_model },
    { "render",     api_render     },
    { "light",      api_light      },
    { "far",        api_far        },
    { "dithering",  api_dithering  },
    { "ambient",    api_ambient    },
    { "snapping",   api_snapping   },
    { "quad",       api_quad       },
    { "rect",       api_rect       },
    { "input",      api_input      },
    { "text",       api_text       },
    { "log",        api_log        },

    { "mouse_down",     api_mouse_down     },
    { "mouse_position", api_mouse_position },
    { "window_size",    api_window_size    },
    { "size",           api_size           },

    { "load_sound",       api_load_sound       },
    { "music_volume",     api_music_volume     },
    { "music_loop",       api_music_loop       },
    { "music_crossfade",  api_music_crossfade  },
    { "music_stop",       api_music_stop       },
    { "music_switch",     api_music_switch     },
    { "music_play",       api_music_play       },
    { "sound_play",       api_sound_play       },
    { "sound_volume",     api_sound_volume     },
    { "sound_pan",        api_sound_pan        },
    { "sound_is_playing", api_sound_is_playing },
    
    { "perlin", api_perlin },

    { "identity", api_identity },
    { "store",    api_store },
    { "retrieve", api_retrieve },

    { "exit", api_exit },

    { NULL, NULL }
};


bool init() {
    luaL_loadstring(l, fs_read("boot.lua", NULL));
    lua_call(l, 0, 0);

    return false;
}

bool tick() {
    lua_getglobal(l, "eng");
    lua_getfield(l, -1, "tick");

    int result = lua_pcall(l, 0, 0, 0);

    lua_pop(l, 1);

    return result != LUA_OK;
}

bool frame(f64 alpha, f64 delta, bool focused) {
    ren_log("\n// ENVIRONMENT //////");

    lua_gc(l, LUA_GCCOLLECT, 0);
    u32 usage = lua_gc(l, LUA_GCCOUNT, 0);
    ren_log("LUA MEMORY: %ukb", usage);
    ren_log("FPS: %i", (u16)(1.0/delta));

    lua_getglobal(l, "eng");
    lua_getfield(l, -1, "frame");

    lua_pushnumber(l, alpha);
    lua_pushnumber(l, delta);
    lua_pushboolean(l, focused);

    int result = lua_pcall(l, 3, 0, 0);

    lua_pop(l, -2);

    return result != LUA_OK;
}

// TODO: Consider embedding the actual Lua library within this specific file, and
//       to fully isolate the engine 

int main(int argc, char *argv[]) {
    printf("setting up environment.\n");

    l = luaL_newstate();
    if(l == NULL)
        return true;

    luaL_openlibs(l);
    
    lua_newtable(l);

    luaL_setfuncs(l, registry, 0);


    #ifdef __linux
        lua_pushstring(l, "linux");
    #elif _WIN32
        lua_pushstring(l, "windows");
    #else
        lua_pushstring(l, "unknown");
    #endif
    lua_setfield(l, -2, "os");


    #ifdef TRASH_DEBUG
        lua_pushboolean(l, true);
    #else
        lua_pushboolean(l, false);
    #endif
    lua_setfield(l, -2, "debug");


    #ifndef THING_COMMIT
    #define THING_COMMIT "unknown"
    #endif

    lua_pushstring(l, THING_COMMIT);
    lua_setfield(l, -2, "commit");


    lua_setglobal(l, "eng");

    int o = eng_main((Application) {
        .init = init,
        .tick = tick,
        .frame = frame
    });

    if (l)
        lua_close(l);

    return o;
}

int WinMain() {
    return main(0, NULL);
}