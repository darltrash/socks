// this implements a basic input system, only keyboard so far

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define BASKET_INTERNAL
#include "basket.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>

#define MAX_CONTROLLERS 16
#define MAX_BINDINGS 8

static u8 bindings_len = 0;
static Binding bindings[MAX_BINDINGS];
static u32 state[INP_MAX];
static u8 current;

static char text[32];
static u16 mouse_x = 0;
static u16 mouse_y = 0;
static bool mouse_button[8];

static SDL_GameController* controllers[MAX_CONTROLLERS];
static i8 focused_controller = -1;

static void open_controller(int index) {
    if (index < 0 || index >= MAX_CONTROLLERS) {
        printf("invalid controller index! (%i)\n", index);
        return;
    }

    if (!SDL_IsGameController(index))
        return;

    controllers[index] = SDL_GameControllerOpen(index);

    if (controllers[index] != NULL)
        printf("controller #%i opened! (%s)\n", index, SDL_GameControllerName(controllers[index]));

    else
        printf("could not open controller #%i! SDL_Error: %s\n", index, SDL_GetError());
}

void close_controller(int instance_id) {
    for (int i = 0; i < MAX_CONTROLLERS; ++i) {
        SDL_GameController *controller = controllers[i];
        if (controller == NULL) continue;

        SDL_Joystick *joy = SDL_GameControllerGetJoystick(controller);
        bool real_instance_id = SDL_JoystickInstanceID(joy);
        if (real_instance_id != instance_id) continue;

        SDL_GameControllerClose(controllers[i]);
        controllers[i] = NULL;

        printf("controller %i closed! (%s)\n", i, SDL_GameControllerName(controllers[i]));

        if (focused_controller == i)
            focused_controller = -1;

        break;
    }
}

void handle_focus_change(int index) {
    if (focused_controller != index) {
        focused_controller = index;
        printf("controller #%i (%s) is now in focus!\n", index, SDL_GameControllerName(controllers[index]));
    }
}

int inp_init() {
    bindings_len = 0;

    u32 num_joysticks = SDL_NumJoysticks();

    for (int i = 0; i < num_joysticks; ++i) {
        open_controller(i);
    }

    for (int i = num_joysticks; i < MAX_CONTROLLERS; ++i) {
        controllers[i] = NULL;
    }

    return 0;
}

void inp_byebye() {
    for (int i = 0; i < MAX_CONTROLLERS; ++i)
        if (controllers[i] != NULL)
            SDL_GameControllerClose(controllers[i]);
}

u8 find_in_binding(Binding bind, u32 btn) {
    for (u8 action = INP_NONE+1; action < INP_MAX; ++action)
        for (int i = 0; i < MAX_BUTTONS; ++i)
            if (bind.buttons[action][i] == btn)
                return action;

    return INP_NONE;
}

static f32 controller_dir[2];

void inp_event(SDL_Event event) {
    switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            if (event.key.repeat)
                return;

            u32 scancode = event.key.keysym.scancode;

            u32 bind = find_in_binding(bindings[current], scancode);

            if (bind == INP_NONE) {
                for (u32 i=0; i < bindings_len; i++) {
                    bind = find_in_binding(bindings[i], scancode);

                    if (bind != INP_NONE) {
                        current = i;
                        break;
                    }
                }
            }

            if (bind == INP_NONE)
                return;

            state[bind] = event.type == SDL_KEYDOWN;

            break;
        }

        case SDL_TEXTINPUT: {
            memcpy(text, event.text.text, 32);
            return;
        }

        case SDL_CONTROLLERDEVICEADDED: {
            open_controller(event.cdevice.which);
            return;
        }

        case SDL_CONTROLLERDEVICEREMOVED: {
            close_controller(event.cdevice.which);
            return;
        }

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START || focused_controller == -1)
                handle_focus_change(event.cbutton.which);

            if (event.cbutton.which != focused_controller) return;

            u32 button = event.cbutton.button + SDL_NUM_SCANCODES;

            u32 binding = find_in_binding(bindings[current], button);

            if (binding == INP_NONE) {
                for (u32 i=0; i < bindings_len; i++) {
                    binding = find_in_binding(bindings[i], button);

                    if (binding != INP_NONE) {
                        current = i;
                        break;
                    }
                }
            }

            if (binding == INP_NONE)
                return;

            state[binding] = event.type == SDL_CONTROLLERBUTTONDOWN;

            return;
        }

        case SDL_CONTROLLERAXISMOTION: {
            if (event.caxis.which != focused_controller) return;
            if (event.caxis.axis >= 2) return;
            controller_dir[event.caxis.axis] = (float)(event.caxis.value) / 32768.0;
            return; // AXIS1:X   AXIS2:Y
        }

        case SDL_MOUSEMOTION: {
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            mouse_button[event.button.button % 8] = true;
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            mouse_button[event.button.button % 8] = false;
            break;
        }


        default: break;
    }
}


void inp_mouse_position(u16 *x, u16 *y) {
    if (x != NULL)
        *x = mouse_x;

    if (y != NULL)
        *y = mouse_y;
}

bool inp_mouse_down(u8 button) {
    return mouse_button[button % 3];
}

bool inp_update(f64 timestep) {
    memset(text, 0, 32);

    for (u32 i = 0; i < INP_MAX; i++)
        if (state[i])
            state[i]++;

    return false;
}

const char *inp_text() {
    return text;
}

u32 inp_button(u8 button) {
    return state[button];
}

//void inp_clear() {
//    vec_clear(&bindings);
//}

bool inp_direction(f32 direction[2]) {
    float rlen = vec_len(controller_dir, 2);
    if (rlen > 0.01) {
        direction[0] = controller_dir[0];
        direction[1] = controller_dir[1];
        return true;
    }

    direction[0] = direction[1] = 0.0;

    if (inp_button(INP_LEFT))
        direction[0] -= 1.0;

    if (inp_button(INP_RIGHT))
        direction[0] += 1.0;

    if (inp_button(INP_UP))
        direction[1] -= 1.0;

    if (inp_button(INP_DOWN))
        direction[1] += 1.0;

    rlen = vec_len(direction, 2);
    if (rlen < 0.1) return false;

    // Normalize
    direction[0] /= rlen;
    direction[1] /= rlen;

    return true;
}

static u32 inp_to_code(const char *name) {
    if (strncmp(name, "gamepad:", 8) == 0)
        return 512 + SDL_GameControllerGetButtonFromString(name + 8);

    if (strncmp(name, "keyboard:", 9) == 0)
        return SDL_GetScancodeFromName(name + 9);

    return SDL_GetScancodeFromName(name);
}

char *inp_from_code(u32 code) {
    const char *str;

    if (code > 512)
        str = SDL_GameControllerGetStringForButton(code - 512);
    else
        str = SDL_GetKeyName(SDL_GetKeyFromScancode(code));

    if (str == NULL || str[0] == 0)
        return NULL;

    char buffer[32];
    snprintf(buffer, 32, code > 512 ? "gamepad:%s" : "keyboard:%s", str);

    return strdup(buffer);
}

static void fill_action(u32 action, char **names, Binding *binding) {
    if (!names) return; // If names are null, skip

    for (int i = 0; i < MAX_BUTTONS && names[i] != NULL; ++i) {
        binding->buttons[action][i] = inp_to_code(names[i]);
    }
}

void inp_bind(RawBindings raw) {
    Binding *binding = &bindings[bindings_len++];
    memset(binding, 0, sizeof(Binding));

    fill_action(INP_UP, raw.up, binding);
    fill_action(INP_DOWN, raw.down, binding);
    fill_action(INP_LEFT, raw.left, binding);
    fill_action(INP_RIGHT, raw.right, binding);
    fill_action(INP_JUMP, raw.jump, binding);
    fill_action(INP_ATTACK, raw.attack, binding);
    fill_action(INP_MENU, raw.menu, binding);
}

Binding inp_current() {
    return bindings[current];
}
