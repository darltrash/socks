// the brain.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

#define BASKET_INTERNAL
#include "common.h"
#include "cute_sound.h"
#include "cute_png.h"

static f64 timestep;
static f64 lag;// = timestep;
static bool instant = false;

static bool running = true;
static bool focused = true;

static u16 window_width  = 840;
static u16 window_height = 630;
static u16 mouse_x = 0;
static u16 mouse_y = 0;
static bool mouse_button[3];

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

        case SDL_MOUSEMOTION: {
            mouse_x = event.motion.x;   
            mouse_y = event.motion.y;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            mouse_button[event.button.button % 3] = true;
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            mouse_button[event.button.button % 3] = false;
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
                    cs_set_global_pause(true);
                    focused = false;
                    break;
                }

                case SDL_WINDOWEVENT_FOCUS_GAINED: {
                    cs_set_global_pause(false);
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
    *w = window_width;
    *h = window_height;
}

void eng_mouse_position(u16 *x, u16 *y) {
    *x = mouse_x;
    *y = mouse_y;
}

bool eng_mouse_down(u8 button) {
    return mouse_button[button % 3];
}

void eng_tickrate(f64 hz) {
    instant = hz == 0.0;
    timestep = 1.0/hz;
}


bool eng_main(Application app, void *userdata) {
    printf("waste basket, keep my brain entertained.\n");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    assert(app.init);
    assert(app.tick);
    assert(app.frame);

  	SDL_Window *window = SDL_CreateWindow(
        "socks",
  	    SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
  	    window_width, window_height,
  	    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
  	);

    // renderer.
    if (ren_init(window))
        return 1;
    
    // audio.
    printf("setting up audio.\n");
    cs_init(NULL, 44100, 1024, NULL);
    cs_spawn_mix_thread();

    cs_set_global_volume(2.0);
    if (getenv("BK_NO_VOLUME"))
        cs_set_global_volume(0.0);

    // input
    inp_setup();

    inp_register_scancode("w", INP_UP    ),
    inp_register_scancode("s", INP_DOWN  ),
    inp_register_scancode("a", INP_LEFT  ),
    inp_register_scancode("d", INP_RIGHT ),

    inp_register_scancode("j", INP_JUMP   ),
    inp_register_scancode("k", INP_ATTACK ),
    inp_register_scancode("l", INP_MENU   ),

    inp_register_scancode("up",    INP_UP    ),
    inp_register_scancode("down",  INP_DOWN  ),
    inp_register_scancode("left",  INP_LEFT  ),
    inp_register_scancode("right", INP_RIGHT ),

    inp_register_scancode("z", INP_JUMP   ),
    inp_register_scancode("x", INP_ATTACK ),
    inp_register_scancode("c", INP_MENU   ),

    // sigh...
    inp_register_scancode("space", INP_JUMP),

    // for our delta stuff
    printf("setting up timer.\n");
    u64 now = SDL_GetPerformanceCounter();
    u64 last = now;

    static f64 deltas[8];
    u8 delta_idx = 0;
    u8 delta_len = 0;

    f64 timer;

    eng_tickrate(30);

    if (app.init(userdata))
        return 1;

    lag = timestep;

    running = true;

    while (running) {
        // calculate delta
        now = SDL_GetPerformanceCounter();
        f64 real_delta = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
        last = now;

        // audio magic
        cs_update(real_delta);

        // delta smoothing:
        deltas[delta_idx++] = real_delta;
        delta_len = max(delta_len, delta_idx);
        if (delta_idx == 8) delta_idx = 0;

        f64 delta = 0.0;
        for (u8 i=0; i < delta_len; i++)
            delta += deltas[i];
        delta /= (f64)delta_len;


        if (instant) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev))
                event(ev, window);

            if (focused)
                app.tick(timestep);

            inp_update(timestep);

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
                    app.tick(timestep);

                inp_update(timestep);

                // computer is too god damn slow, sorry.
                if (n++ == 3) {
                    lag = 0.0;
                    printf("Could not hit %.2fhz!\n", 1.0/timestep);
                    break;
                }
            }

        }

        if (app.frame(lag / timestep, delta, focused))
            return 1;

        if (!focused) {
            u16 w, h; 
            ren_size(&w, &h);

            ren_rect(-w, -w, h, h, (Color){ .full = 0x000000FF });
        }

        if (ren_frame())
            return 1;

        SDL_GL_SwapWindow(window);
    }

    ren_byebye();
    printf("byebye says the window.\n");
    SDL_DestroyWindow(window);

    printf("byebye says the music.\n");
    cs_shutdown();

    printf("byebye says the world.\n");
    SDL_Quit();

    printf("the end.\n");

    if (app.close)
        app.close();

    return 0;
}