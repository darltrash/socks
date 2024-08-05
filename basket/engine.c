// the brain.

#include <SDL2/SDL_messagebox.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

#define BASKET_INTERNAL
#include "basket.h"

static f64 hz;

static bool running = true;
static bool focused = true;

static u16 window_width  = 960;
static u16 window_height = 700;
static bool is_debug = false;

void event(SDL_Event event, SDL_Window *window) {
    static bool fullscreen = false;

    inp_event(event);

    switch (event.type) {
        case SDL_QUIT: {
            running = false;

            break;
        }

        case SDL_KEYDOWN: {
            switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_F11: {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window, fullscreen);
                }

                default: break;
            }
            break;
        }

        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                    break;
                }

                case SDL_WINDOWEVENT_FOCUS_LOST: {
                    // cs_set_global_pause(true);
                    focused = false;
                    break;
                }

                case SDL_WINDOWEVENT_FOCUS_GAINED: {
                    // cs_set_global_pause(false);
                    focused = true;
                    break;
                }

                default: break;
            }
        }

        default: break;
    }
}

void eng_close() {
    running = false;
}


// TODO: This shit is not future proof.
void eng_window_size(u16 *w, u16 *h) {
    if (w != NULL)
        *w = window_width;

    if (h != NULL)
        *h = window_height;
}


void eng_tickrate(f64 _hz) {
    hz = _hz;
}

bool eng_is_focused() {
    return focused;
}

void eng_set_debug(bool debug) {
    is_debug = debug;
}

bool eng_is_debug() {
    return is_debug;
}

const char *eng_error_string(int error) {
    switch (error) {
        case BSKT_UNKNOWN:                  return "Unknown error!";
        case BSKT_FS_COULDNT_FIND_PACKAGE:  return "Could not find package!";
        case BSKT_SAV_COULDNT_SAVE:         return "Could not save to savefile!";
        case BSKT_MOD_UNRECOGNIZED_FORMAT:  return "Unrecognized model format!";
        case BSKT_IMG_COULDNT_LOAD_IMAGE:   return "Could not load image!";
        case BSKT_REN_COULDNT_INIT:         return "Could not initialize renderer!";
        case BSKT_REN_COULDNT_RENDER_FRAME: return "Could not render frame!";
    }

    return NULL;
}

#define ENG_CALL_IF_VALID(func, ...) { \
    ret = 0;                           \
    if (func)                          \
        ret = func(__VA_ARGS__);       \
                                       \
    if (ret)                           \
        goto general_error;            \
}

static void register_simple_controls(
    const char *up, const char *down, const char *left, const char *right,
    const char *jump, const char *attack, const char *menu
) {

}

int eng_main(Application app, const char *arg0) {
    printf("hello world, i'm basket.\n");

    SDL_Init( 0
        | SDL_INIT_VIDEO
        | SDL_INIT_EVENTS
        | SDL_INIT_AUDIO
        | SDL_INIT_JOYSTICK
        | SDL_INIT_GAMECONTROLLER
    );

  	SDL_Window *window = SDL_CreateWindow(
        "socks",
  	    SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
  	    window_width, window_height,
  	    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
  	);

    if (window == NULL)
        goto window_init_error;

    int ret = 0;

    // filesystem.
    ret = fs_init(arg0);
    if (ret)
        goto fs_init_error;

    // audio.
    ret = aud_init();
    if (ret)
        goto aud_init_error;

    // renderer.
    ret = ren_init(window);
    if (ret)
        goto ren_init_error;

    // input
    ret = inp_init();
    if (ret)
        goto inp_init_error;

    inp_bind((RawBindings) {
        .up     = (char *[]) {"w", 0},
        .down   = (char *[]) {"s", 0},
        .left   = (char *[]) {"a", 0},
        .right  = (char *[]) {"d", 0},

        .jump   = (char *[]) {"j", "space", 0},
        .attack = (char *[]) {"k", 0},
        .menu   = (char *[]) {"l", 0}
    });

    inp_bind((RawBindings) {
        .up     = (char *[]) {"up"   , 0},
        .down   = (char *[]) {"down" , 0},
        .left   = (char *[]) {"left" , 0},
        .right  = (char *[]) {"right", 0},

        .jump   = (char *[]) {"z", "space", 0},
        .attack = (char *[]) {"x", 0},
        .menu   = (char *[]) {"c", 0}
    });

    inp_bind((RawBindings) {
        .up     = (char *[]) {"gamepad:dpup",    0},
        .down   = (char *[]) {"gamepad:dpdown",  0},
        .left   = (char *[]) {"gamepad:dpleft",  0},
        .right  = (char *[]) {"gamepad:dpright", 0},

        .jump   = (char *[]) {"gamepad:a", 0},
        .attack = (char *[]) {"gamepad:b", 0},
        .menu   = (char *[]) {"gamepad:x", 0}
    });

    eng_tickrate(30);

    ENG_CALL_IF_VALID(app.init)

    // for our delta stuff
    printf("setting up timer.\n");
    u64 now = SDL_GetPerformanceCounter();
    u64 last = now;

    static f64 deltas[8];
    u8 delta_idx = 0;
    u8 delta_len = 0;

    f32 lag = 1.0/hz;

    while (running) {
        f32 timestep = 1.0/hz;

        // calculate delta
        now = SDL_GetPerformanceCounter();
        f64 real_delta = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
        last = now;

        // delta smoothing:
        deltas[delta_idx++] = real_delta;
        delta_len = max(delta_len, delta_idx);
        if (delta_idx == 8) delta_idx = 0;

        f64 delta = 0.0;
        for (u8 i=0; i < delta_len; i++)
            delta += deltas[i];
        delta /= (f64)delta_len;

        if (hz == 0) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev))
                event(ev, window);

            if (focused)
                ENG_CALL_IF_VALID(app.tick, delta)

            inp_update(delta);

        } else {
            // fixed timesteps
            lag += real_delta;
            u8 n = 0;
            while (lag > timestep) {
                lag -= timestep;

                // input polling! (delicious)
                SDL_Event ev;
                while (SDL_PollEvent(&ev))
                    event(ev, window);

                if (focused)
                    ENG_CALL_IF_VALID(app.tick, timestep)

                inp_update(timestep);

                // computer is too god damn slow, sorry.
                if (n++ == 3) {
                    lag = 0.0;
                    printf("Could not hit %.1fhz!\n", hz);
                    break;
                }
            }

        }

        ENG_CALL_IF_VALID(app.frame, lag / timestep, delta)

        if (!focused) {
            u16 w, h;
            ren_size(&w, &h);

            ren_rect(-(i32)(w/2), -(i32)(w/2), h*2, h*2, (Color){ .full = 0x00000055 });
        }

        if (ren_frame())
            return 1;

        SDL_GL_SwapWindow(window);
    }


    general_error:
    printf("byebye says the sensorial.\n");
    inp_byebye();
    inp_init_error:

    printf("byebye says the music.\n");
    aud_byebye();
    aud_init_error:

    printf("byebye says the illusion.\n");
    ren_byebye();
    ren_init_error:
    fs_init_error:

    if (app.close)
        ret = app.close(ret);

    const char *msg = eng_error_string(ret);
    if (msg != NULL)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", msg, window);

    printf("byebye says the window.\n");
    SDL_DestroyWindow(window);
    window_init_error:

    printf("byebye says the world.\n");
    SDL_Quit();

    printf("the end.\n");


    return ret;
}
