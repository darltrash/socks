// implements the lua api that pilots this huge mech of a tech stack.

#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_VORBIS_INCLUDE_STB_VORBIS_H 1

#include "basket.h"
#include "lib/minilua.h"
#include "lib/stb_perlin.h"

static lua_State *L;

static int api_noop(lua_State *l) { return 0; }

static void get_vector(lua_State *l, f32 *out, int tableIndex, int len) {
    luaL_checktype(l, tableIndex, LUA_TTABLE);

    for (size_t i = 0; i < len; ++i) {
        lua_pushnumber(l, i+1);
        lua_gettable(l, tableIndex);
        out[i] = luaL_checknumber(l, -1);

        lua_pop(l, 1);
    }
}

static int require_fs_read(lua_State *l) {
    const char* module = luaL_checkstring(l, 1);

    for (char* p = (char *)module; *p; p++)
        if (*p == '.')
            *p = '/';

    char path[256];
    snprintf(path, sizeof(path), "%s.lua", module);

    u32 length;
    const char* source = fs_read(path, &length);

    if (source) {
        if (luaL_loadbuffer(l, source, length, path) != LUA_OK) {
            return lua_error(l);
        }
        return 1;
    }

    // TODO: TIDY IT UP

    snprintf(path, sizeof(path), "%s/init.lua", module);
    source = fs_read(path, &length);

    if (source) {
        if (luaL_loadbuffer(l, source, length, path) != LUA_OK) {
            return lua_error(l);
        }
        return 1;
    }

    lua_pushnil(l);
    lua_pushfstring(l, "Module '%s' could not be read by fs_read()!", module);
    return 2;
}

static void add_package_searcher(lua_State *l) {
    lua_getglobal(l, "package");
    lua_getfield(l, -1, "searchers");

    lua_pushcfunction(l, require_fs_read);
    for (int i = (int)lua_rawlen(l, -2) + 1; i > 1; i--) {
        lua_rawgeti(l, -2, i - 1);
        lua_rawseti(l, -3, i);
    }
    lua_rawseti(l, -2, 1);

    lua_pop(l, 2);
}

static int api_reload_tex(lua_State *l) {
    u32 length = 0;
    static u8 main, lumos;
    if (main)
        ren_tex_free(main);

    if (lumos)
        ren_tex_free(lumos);

    {
        char *raw = fs_read("assets/tex_atlas.png", &length);
        main = ren_tex_load(raw, length);
        free(raw);
    }

    {
        char *raw = fs_read("assets/tex_lumos.png", &length);
        lumos = ren_tex_load(raw, length);
        free(raw);
    }

    ren_tex_bind(main, lumos);

    return 0;
}

static int api_load_image(lua_State *l) {
    u32 length;
    char *str = fs_read(luaL_checkstring(l, 1), &length);

    if (str == NULL)
        return 0;

    Image img;
    if (img_init(&img, str, length))
        return 0;

    lua_newtable(l);
    lua_pushlstring(l, (void*)img.pixels, img.w * img.h * sizeof(Color));
    lua_setfield(l, -2, "pixels");
    lua_pushinteger(l, img.w);
    lua_setfield(l, -2, "w");
    lua_pushinteger(l, img.h);
    lua_setfield(l, -2, "h");

    return 1;
}

// eng.read(filename) -> string?
static int api_fs_read(lua_State *l) {
    u32 size;
    char *str = fs_read(luaL_checkstring(l, 1), &size);

    if (str == NULL)
        lua_pushnil(l);

    else
        lua_pushlstring(l, str, size);

    free(str);

    return 1;
}

static int api_videomode(lua_State *l) {
    ren_videomode(
        luaL_checknumber(l, 1),
        luaL_checknumber(l, 2),
        lua_toboolean(l, 3)
    );

    return 0;
}

// eng.camera(from, to)
static int api_camera(lua_State *l) {
    f32 a[3];
    f32 b[3];
    f32 c[3];
    get_vector(l, a, 1, 3);
    get_vector(l, b, 2, 3);
    get_vector(l, c, 3, 3);

    ren_camera(a, b, c);

    return 0;
}

// eng.load_model(filename) -> { begin, length }
static int api_load_model(lua_State *l) {
    const char *file = luaL_checkstring(l, 1);

    u32 size;
    char *str = fs_read(file, &size);

    if (str == NULL) {
        luaL_error(l, "file doesnt exist, this one -> \"%s\"", file);
        return 0;
    }

    Model model;

    if (mod_init(&model, str)) {
        luaL_error(l, "could not decode! are you sure this is valid data");
        return 0;
    }

    free(str);

    lua_newtable(l);
    lua_pushlstring(l, (void*)model.mesh.data, model.mesh.length * sizeof(Vertex));
    lua_setfield(l, -2, "data");
    lua_pushinteger(l, model.mesh.length);
    lua_setfield(l, -2, "length");
    if (model.extra) {
        lua_pushstring(l, model.extra);
        lua_setfield(l, -2, "extra");
    }

    lua_newtable(l);
    for (u32 i = 0; i < model.submesh_amount; i++) {
        SubMesh *submesh = &model.submeshes[i];

        lua_newtable(l);
        lua_pushinteger(l, submesh->range.offset);
        lua_rawseti(l, -2, 1);
        lua_pushinteger(l, submesh->range.length);
        lua_rawseti(l, -2, 2);

        lua_setfield(l, -2, submesh->name);
    }

    lua_setfield(l, -2, "submeshes");

    mod_free(&model);

    return 1;
}


static int api_load_sound(lua_State *l) {
    Sound snd;

    const char *file = luaL_checkstring(l, 1);

    u32 size;
    char *str = fs_read(file, &size);

    if (str == NULL) {
        luaL_error(l, "file doesnt exist, this one -> \"%s\"", file);
        return 0;
    }

    Sound sound;
    int err = aud_load_ogg(&snd, (u8 *)str, size, lua_toboolean(l, 2));

    free(str);

    if (err) {
        luaL_error(l, "error loading source \"%s\"", file);
        return 0;
    }

    lua_pushinteger(l, snd);

    return 1;
}

static int api_listener(lua_State *l) {
    f32 position[3];
    get_vector(l, position, 1, 3);
    aud_listener(position);
    return 0;
}

static int api_orientation(lua_State *l) {
    f32 orientation[6];
    f32 up[6];
    get_vector(l, orientation, 1, 3);
    get_vector(l, up, 2, 3);

    aud_orientation(orientation, up);
    return 0;
}


static Color fetch_color(lua_State *l, int a) {
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

static TextureSlice fetch_texture(lua_State *l, int a) {
    TextureSlice texture = { 0, 0, 0, 0 };

    if (!lua_isnil(l, a)) {
        luaL_checktype(l, a, LUA_TTABLE);

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

static Range fetch_range(lua_State *l, int a) {
    Range range = { 0, 0 };

    if (!lua_isnil(l, a)) {
        luaL_checktype(l, a, LUA_TTABLE);

        static u16 tmp[2];

        for (int i = 1; i <= 2; ++i) {
            lua_rawgeti(l, a, i);
            luaL_checkinteger(l, -1);
            tmp[i - 1] = lua_tointeger(l, -1);
            lua_pop(l, 1);
        }

        range.offset = tmp[0];
        range.length = tmp[1];
    }

    return range;
}


static int fetch_call(lua_State *l, RenderCall *call) {
    luaL_checktype(l, 1, LUA_TTABLE);

    lua_getfield(l, 1, "disable");
    if (!lua_isnil(l, -1))
        call->disable = lua_toboolean(l, -1);

    lua_getfield(l, 1, "model");
    if (!lua_isnil(l, -1)) {
        luaL_checktype(l, -1, LUA_TTABLE);

        for (int i=1; i <= 16; i++) {
            lua_pushinteger(l, i);
            lua_gettable(l, -2);
            call->model[i-1] = (f32)lua_tonumber(l, -1);

            lua_pop(l, 1);
        }
    }

    lua_getfield(l, 1, "tint");
    call->tint = fetch_color(l, -1);

    lua_getfield(l, 1, "mesh");
    if (!lua_istable(l, -1)) {
        luaL_error(l, "'mesh' property must be a table");
        return 1;
    }

        lua_getfield(l, -1, "data");
        call->mesh.data = (void*)lua_tostring(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, -1, "length");
        call->mesh.length = lua_tointeger(l, -1);
        lua_pop(l, 1);

    lua_getfield(l, 1, "texture");
    call->texture = fetch_texture(l, -1);

    lua_getfield(l, 1, "range");
    call->range = fetch_range(l, -1);

    return 0;
}

// eng.render(render_call)
static int api_render(lua_State *l) {
    RenderCall call = { false, IDENTITY_MATRIX, COLOR_WHITE, {}, {0, 0, 0, 0}, NULL };
    if (fetch_call(l, &call))
        return 0;

    ren_render(call);

    return 0;
}

// eng.draw(render_call)
static int api_draw( lua_State *l) {
    RenderCall call = { false, IDENTITY_MATRIX, COLOR_WHITE, {}, {0, 0, 0, 0}, NULL };

    if (fetch_call(l, &call))
        return 0;

    ren_draw(call);

    return 0;
}

// eng.light(pos, color)
static int api_light(lua_State *l) {
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


static int api_far(lua_State *l) {
    ren_far(luaL_checknumber(l, 1), fetch_color(l, 2));
    return 0;
}

static int api_dithering(lua_State *l) {
    ren_dithering(lua_toboolean(l, 1));
    return 0;
}

static int api_ambient(lua_State *l) {
    ren_ambient(fetch_color(l, 1));
    return 0;
}

static int api_snapping(lua_State *l) {
    ren_snapping(luaL_checknumber(l, 1));
    return 0;
}



// ren.quad(texture, x, y, color)
static int api_quad(lua_State *l) {
    Quad quad = DEFAULT_QUAD;

    quad.texture = fetch_texture(l, 1);
    quad.position[0] = lua_tonumber(l, 2);
    quad.position[1] = lua_tonumber(l, 3);
    quad.color = fetch_color(l, 4);

    if (!lua_isnoneornil(l, 5))
        quad.scale[0] = quad.scale[1] = lua_tonumber(l, 5);

    if (!lua_isnoneornil(l, 6))
        quad.scale[1] = lua_tonumber(l, 6);


    ren_quad(quad);

    return 0;
}

// ren.rect(x, y, w, h, color)
static int api_rect( lua_State *l) {
    ren_rect(
        lua_tonumber(l, 1),
        lua_tonumber(l, 2),
        lua_tonumber(l, 3),
        lua_tonumber(l, 4),
        fetch_color(l, 5)
    );

    return 0;
}

// X, Y, MOVING
static int api_direction(lua_State *l) {
    f32 direction[2];
    bool moving = inp_direction(direction);

    lua_pushnumber(l, direction[0]);
    lua_pushnumber(l, direction[1]);
    lua_pushboolean(l, moving);

    return 3;
}

// KEEP THIS SYNCED WITH COMMON.H
const char *buttons[] = {
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

static int api_input(lua_State *l) {
    int e = luaL_checkoption(l, 1, NULL, buttons);
    int o = inp_button(e);

    if (o)
        lua_pushinteger(l, o);
    else
        lua_pushnil(l);

    return 1;
}

static int api_current_bind(lua_State *l) {
    Binding current = inp_current();

    lua_newtable(l);

    for (int action = 0; action < INP_MAX; action++) {
        lua_pushstring(L, buttons[action]);
        lua_newtable(L);

        for (int j = 0; j < MAX_BUTTONS; ++j) {
            char *data = inp_from_code(current.buttons[action][j]);
            if (data == NULL) break;

            lua_pushinteger(L, j + 1);
            lua_pushstring(L, data);
            lua_settable(L, -3);

            free(data);
        }

        // Pop the table and set it in the main table
        lua_settable(L, -3);
    }

    return 1;
}


static int api_text( lua_State *l) {
    lua_pushstring(l, inp_text());
    return 1;
}

static int api_log( lua_State *l) {
    size_t _ = 0;
    ren_log("%s", luaL_tolstring(l, 1, &_));

    return 0;
}

static int api_mouse_down( lua_State *l) {
    bool down = inp_mouse_down(luaL_checkinteger(l, 1));
    lua_pushboolean(l, down);

    return 1;
}

static int api_mouse_position( lua_State *l) {
    i16 x, y;
    ren_mouse_position(&x, &y);

    lua_pushinteger(l, x);
    lua_pushinteger(l, y);

    return 2;
}

static int api_raw_mouse_position( lua_State *l) {
    u16 x, y;
    inp_mouse_position(&x, &y);

    lua_pushinteger(l, x);
    lua_pushinteger(l, y);

    return 2;
}

static int api_window_size( lua_State *l) {
    u16 w, h;
    eng_window_size(&w, &h);

    lua_pushinteger(l, w);
    lua_pushinteger(l, h);

    return 2;
}

static int api_size( lua_State *l) {
    u16 w, h;
    ren_size(&w, &h);

    lua_pushinteger(l, w);
    lua_pushinteger(l, h);

    return 2;
}

static int api_sound_state( lua_State *l) {
    Sound snd = luaL_checkinteger(l, 1);
    int state = aud_state(snd);

    switch (state) {
        case AUD_STATE_INITIAL: lua_pushstring(l, "initial"); break;
        case AUD_STATE_STOPPED: lua_pushstring(l, "stopped"); break;
        case AUD_STATE_PLAYING: lua_pushstring(l, "playing"); break;
        case AUD_STATE_PAUSED:  lua_pushstring(l, "paused");  break;
        default: {
            luaL_error(l, "Internal error in aud_state! :(");
            return 0;
        }
    }

    return 1;
}

// eng.sound_play(source, params) -> sound
static int api_sound_play(lua_State *l) {
    Sound snd = luaL_checkinteger(l, 1);

    Source src;
    if (aud_init_source(&src, snd)) {
        luaL_error(l, "Could not initialize source.");
        return 0;
    }

    if (!lua_isnoneornil(l, 2)) {
        luaL_checktype(l, 2, LUA_TTABLE);

        static f32 v[3];

        //lua_getfield(l, 2, "paused");
        //if (!lua_isnil(l, -1))
        //    aud_pause(src, lua_toboolean(l, -1));
        //lua_pop(l, 1);

        lua_getfield(l, 2, "looping");
        if (!lua_isnil(l, -1))
            aud_set_looping(src, lua_toboolean(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, 2, "gain");
        if (!lua_isnil(l, -1))
            aud_set_gain(src, luaL_checknumber(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, 2, "area");
        if (!lua_isnil(l, -1))
            aud_set_area(src, luaL_checknumber(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, 2, "pitch");
        if (!lua_isnil(l, -1))
            aud_set_pitch(src, luaL_checknumber(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, 2, "position");
            if (!lua_isnil(l, -1)) {
                for (size_t i = 0; i < 3; ++i) {
                    lua_pushnumber(l, i+1);
                    lua_gettable(l, -2);
                    v[i] = luaL_checknumber(l, -1);

                    lua_pop(l, 1);
                }
                aud_set_position(src, v);
            }
        lua_pop(l, 1);

        lua_getfield(l, 2, "velocity");
            if (!lua_isnil(l, -1)) {
                for (size_t i = 0; i < 3; ++i) {
                    lua_pushnumber(l, i+1);
                    lua_gettable(l, -2);
                    v[i] = luaL_checknumber(l, -1);

                    lua_pop(l, 1);
                }
                aud_set_velocity(src, v);
            }
        lua_pop(l, 1);
    }

    aud_play(src);

    lua_pushinteger(l, src);
    return 1;
}

static int api_sound_gain( lua_State *l) {
    aud_set_gain(luaL_checkinteger(l, 1), luaL_checknumber(l, 2));
    return 1;
}

static int api_sound_pitch( lua_State *l) {
    aud_set_pitch(luaL_checkinteger(l, 1), luaL_checknumber(l, 2));
    return 1;
}

static int api_sound_area( lua_State *l) {
    aud_set_area(luaL_checkinteger(l, 1), luaL_checknumber(l, 2));
    return 1;
}

static int api_sound_stop( lua_State *l) {
    aud_stop(luaL_checkinteger(l, 1));
    return 1;
}

static int api_perlin( lua_State *l) {
    float x = luaL_checknumber(l, 1);
    float y = luaL_checknumber(l, 2);
    float z = luaL_checknumber(l, 3);

    int seed = 0x1337C0DE;
    if (!lua_isnoneornil(l, 4))
        seed = luaL_checkinteger(l, 4);

    lua_pushnumber(l, stb_perlin_noise3_seed(x, y, z, 0, 0, 0, seed));
    return 1;
}

static int api_identity( lua_State *l) {
    int o = sav_identity(lua_tostring(l, 1));
    if (o)
        luaL_error(l, "Could not set savefile identity!");

    return 0;
}

static int api_store( lua_State *l) {
    size_t size;
    const char *str = luaL_checklstring(l, 1, &size);
    int o = sav_store(str, size);
    if (o)
        luaL_error(l, "Could not store into savefile!");

    return 0;
}

static int api_retrieve(lua_State *l) {
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

static int api_exit(lua_State *l) {
    eng_close();
    return 0;
}

static int api_wait(lua_State *l) {
    SDL_Delay(luaL_checknumber(l, 1) * 1000.0);
    return 0;
}

typedef struct {
    SDL_Thread *thread;
    lua_State *L;
    int result_ref;
    bool finished;
    bool in_use;
} ThreadData;

#define MAX_THREADS 256
static ThreadData thread_data[MAX_THREADS];

static int thread_function(void *data) {
    ThreadData *tdata = (ThreadData *)data;

    lua_getglobal(tdata->L, "thread_function");

    if (lua_pcall(tdata->L, 0, 1, 0) != LUA_OK) {
        fprintf(stderr, "Error running Lua function: %s\n", lua_tostring(tdata->L, -1));
        lua_pop(tdata->L, 1);
        tdata->finished = true;
        return -1;
    }

    // Store the result reference
    tdata->result_ref = luaL_ref(tdata->L, LUA_REGISTRYINDEX);
    tdata->finished = true;
    return 0;
}

// Copy a Lua value from one state to another
static void copy_value(lua_State *from, lua_State *to, int index) {
    switch (lua_type(from, index)) {
        case LUA_TNUMBER:
            lua_pushnumber(to, lua_tonumber(from, index));
            break;
        case LUA_TSTRING: {
            size_t l;
            const char *str = luaL_checklstring(from, index, &l);
            lua_pushlstring(to, str, l);
            break;
        }
        case LUA_TBOOLEAN:
            lua_pushboolean(to, lua_toboolean(from, index));
            break;
        case LUA_TTABLE:
            lua_newtable(to);
            lua_pushnil(from);  // first key
            while (lua_next(from, index - 1)) {
                copy_value(from, to, -2);  // copy key
                copy_value(from, to, -1);  // copy value
                lua_settable(to, -3);
                lua_pop(from, 1);  // remove value, keep key for next iteration
            }
            break;
        default:
            lua_pushnil(to);
    }
}

// Offload a function to a new thread
static int api_offload(lua_State *l) {
    u32 thread_id;

    ThreadData *tdata = NULL;

    // Find a free thread slot
    for (thread_id = 0; thread_id < MAX_THREADS; thread_id++) {
        if (!thread_data[thread_id].in_use) {
            tdata = &thread_data[thread_id];
            break;
        }
    }

    if (tdata == NULL) {
        lua_pushnil(l);
        lua_pushstring(l, "Too many threads");
        return 2; // Error: Too many threads
    }

    // Get the function code as a string
    size_t len;
    const char *function_code = luaL_checklstring(l, 1, &len);

    // Create a new Lua state for the thread
    tdata->L = luaL_newstate();
    luaL_openlibs(tdata->L);

    add_package_searcher(l);

    static const luaL_Reg registry[] = {
        { "read",       api_fs_read    },
        { "load_sound", api_load_sound },
        { "load_image", api_load_image },
        { "load_model", api_load_model },
        { "perlin",     api_perlin     },
        { "wait",       api_wait       },

        { 0, 0 }
    };

    lua_newtable(tdata->L);
    luaL_setfuncs(tdata->L, registry, 0);
    lua_setglobal(tdata->L, "eng");

    // Load the function code into the new Lua state
    if (luaL_loadbuffer(tdata->L, function_code, len, "thread_function") != LUA_OK) {
        lua_pushnil(l);
        lua_pushstring(l, lua_tostring(tdata->L, -1));
        lua_close(tdata->L);
        return 2; // Error: Failed to load function code
    }

    // Set the function as a global in the new Lua state
    lua_setglobal(tdata->L, "thread_function");

    // Initialize thread state
    tdata->finished = false;
    tdata->result_ref = LUA_NOREF;
    tdata->in_use = true;

    // Create the thread
    tdata->thread = SDL_CreateThread(thread_function, "WorkerThread", tdata);
    if (!tdata->thread) {
        tdata->in_use = false;
        lua_close(tdata->L);  // Clean up Lua state
        lua_pushnil(l);
        lua_pushstring(l, "Failed to create thread");
        return 2; // Error: Failed to create thread
    }

    lua_pushinteger(l, thread_id);
    return 1; // Return the thread ID
}

// Fetch the result of a thread's execution if it is finished
static int api_fetch(lua_State *l) {
    u32 thread_id = luaL_checkinteger(l, 1);
    if (thread_id >= MAX_THREADS || !thread_data[thread_id].in_use) {
        lua_pushnil(l);
        lua_pushstring(l, "Invalid thread ID");
        return 2; // Error: Invalid thread ID
    }

    ThreadData *tdata = &thread_data[thread_id];

    // Check if the thread is finished
    if (tdata->finished) {
        // Retrieve the result from the thread's Lua state
        lua_rawgeti(tdata->L, LUA_REGISTRYINDEX, tdata->result_ref);

        // Transfer the result back to the main Lua state
        copy_value(tdata->L, l, -1);
        lua_pop(tdata->L, 1);  // pop the value from the thread's state

        // Clean up and mark the slot as available for reuse
        SDL_WaitThread(tdata->thread, NULL);
        luaL_unref(tdata->L, LUA_REGISTRYINDEX, tdata->result_ref);
        lua_close(tdata->L);
        tdata->in_use = false;

        return 1; // Return the result
    }

    // Thread is not finished
    lua_pushnil(l);
    return 1;
}


static const luaL_Reg full_registry[] = {
    { "read",       api_fs_read    },
    { "load_sound", api_load_sound },
    { "load_image", api_load_image   },
    { "load_model", api_load_model },
    { "perlin",     api_perlin     },

    { "offload",    api_offload    },
    { "fetch",      api_fetch      },

    { "wait",       api_wait       },

    { "reload_tex",   api_reload_tex   },
    { "videomode",    api_videomode    },
    { "camera",       api_camera       },
    { "render",       api_render       },
    { "draw",         api_draw         },
    { "light",        api_light        },
    { "far",          api_far          },
    { "dithering",    api_dithering    },
    { "ambient",      api_ambient      },
    { "snapping",     api_snapping     },
    { "quad",         api_quad         },
    { "rect",         api_rect         },
    { "direction",    api_direction    },
    { "input",        api_input        },
    { "current_bind", api_current_bind },
    { "text",         api_text         },
    { "log",          api_log          },

    { "mouse_down",         api_mouse_down         },
    { "mouse_position",     api_mouse_position     },
    { "raw_mouse_position", api_raw_mouse_position },
    { "window_size",        api_window_size        },
    { "size",               api_size               },

    { "sound_play",  api_sound_play       },
    { "sound_gain",  api_sound_gain       },
    { "sound_pitch", api_sound_pitch      },
    { "sound_state", api_sound_state      },
    { "sound_stop",  api_sound_stop       },
    { "listener",    api_listener         },
    { "orientation", api_orientation      },

    { "identity", api_identity },
    { "store",    api_store },
    { "retrieve", api_retrieve },

    { "exit", api_exit },

    { "tick",  api_noop },
    { "frame", api_noop },
    { "close", api_noop },

    { NULL, NULL }
};


static int init(void *userdata) {
    u32 length = 0;
    char *boot_lua = fs_read("boot.lua", &length);
    luaL_loadbuffer(L, boot_lua, length, "boot.lua");
    lua_call(L, 0, 0);
    free(boot_lua);

    api_reload_tex(L);

#ifdef TRASH_DEBUG
    eng_set_debug(true);
#endif

    return false;
}

static int tick(f64 timestep) {
    lua_getglobal(L, "eng");
    lua_getfield(L, -1, "tick");

    lua_pushnumber(L, timestep);

    int result = lua_pcall(L, 1, 0, 0);

    lua_pop(L, 1);

    return result != LUA_OK;
}

static int frame(f64 alpha, f64 delta) {
    ren_log("\n// ENVIRONMENT //////");

    //lua_gc(l, LUA_GCCOLLECT, 0);
    u32 usage = lua_gc(L, LUA_GCCOUNT, 0);
    ren_log("LUA MEMORY: %ukb", usage);
    ren_log("FPS: %u", (u16)(1.0/delta));

    lua_getglobal(L, "eng");
    lua_getfield(L, -1, "frame");

    lua_pushnumber(L, alpha);
    lua_pushnumber(L, delta);
    lua_pushboolean(L, eng_is_focused());

    int result = lua_pcall(L, 3, 0, 0);

    lua_pop(L, -2);

    return result != LUA_OK;
}

static int close(int ret) {
    lua_getglobal(L, "eng");
    lua_getfield(L, -1, "close");

    int result = lua_pcall(L, 0, 0, 0);

    lua_pop(L, 1);

    lua_close(L);

    return 0;
}

// TODO: Consider embedding the actual Lua library within this specific file, and
//       to fully isolate the engine

Application app = {
    .init = init,
    .tick = tick,
    .frame = frame,
    .close = close
};

int main(int argc, char *argv[]) {
    //freopen("output.txt", "a+", stdout);

    printf("setting up environment.\n");

    L = luaL_newstate();
    if(L == NULL)
        return true;

    luaL_openlibs(L);
    lua_newtable(L);
    luaL_setfuncs(L, full_registry, 0);

    #ifdef __linux
        lua_pushstring(L, "linux");
    #elif _WIN32
        lua_pushstring(L, "windows");
    #else
        lua_pushstring(L, "unknown");
    #endif
    lua_setfield(L, -2, "os");


    lua_newtable(L);
    for (int i = 0; i < argc; i++) {
        lua_pushnumber(L, i);
        lua_pushstring(L, argv[i]);
        lua_settable(L, -3);
    }
    lua_setfield(L, -2, "args");


    lua_pushboolean(L, eng_is_debug());
    lua_setfield(L, -2, "debug");

    #ifndef BASKET_COMMIT
    #define BASKET_COMMIT "unknown"
    #endif

    lua_pushstring(L, BASKET_COMMIT);
    lua_setfield(L, -2, "commit");

    lua_setglobal(L, "eng");

    add_package_searcher(L);

    int o = eng_main(app, argv[0]);

    return o;
}

int WinMain() {
    return main(0, NULL);
}
