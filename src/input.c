// this implements a basic input system, only keyboard so far

#include <stdio.h>
#include "common.h"
#include "vec.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>


static u32 state[INP_MAX];

static vec_t(InputBinding) bindings;

void inp_setup() {
    if (bindings.data) 
        vec_deinit(&bindings);

    vec_init(&bindings);
}

void inp_event(SDL_Event event) {
    switch (event.type) {
        case SDL_KEYDOWN: 
        case SDL_KEYUP: {
            if (event.key.repeat) 
                return;

            for (u32 i = 0; i < bindings.length; i++) {
                InputBinding b = bindings.data[i];
                if (!b.binding) continue;

                if (event.key.keysym.scancode == b.code) {
                    state[b.binding] = event.type == SDL_KEYDOWN;
                    return;
                }
            }
            break;
        }

        case SDL_JOYAXISMOTION: {
            printf("not supported\n");
            break;
        }

        case SDL_JOYBUTTONDOWN: 
        case SDL_JOYBUTTONUP: {
            printf("not supported\n");
            break;
        }

        default: break;
    }
}

bool inp_update(f64 delta) {
    for (u32 i = 0; i < INP_MAX; i++)
        if (state[i])
            state[i]++;
    
    return false;
}

u32 inp_button(u8 button) {
    return state[button];
}

void inp_clear() {
    vec_clear(&bindings);
}

static bool inp_register(SDL_Scancode scancode, u8 button) {
    if (scancode == SDL_SCANCODE_UNKNOWN)
        return false;

    InputBinding binding = { button, scancode };

    vec_push(&bindings, binding);

    return true;
}

bool inp_register_scancode(const char *scancode, u8 button) {
    SDL_Scancode code = SDL_GetScancodeFromName(scancode);
    
    return inp_register(code, button);
}

bool inp_register_keycode(const char *keycode, u8 button) {
    SDL_Scancode code = SDL_GetScancodeFromKey(SDL_GetKeyFromName(keycode));

    return inp_register(code, button);
}

// TODO: Somehow handle several keys assigned to the same button 
const char *inp_get_key(u8 button) {
    for (u8 i = 0; i < bindings.length; i++) {
        InputBinding binding = bindings.data[i];
        if (!binding.code) break;

        if (binding.binding == button)
            return SDL_GetKeyName(SDL_GetKeyFromScancode(binding.code));

        i++;
    }

    return NULL;
}