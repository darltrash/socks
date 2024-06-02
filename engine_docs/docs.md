# BASKET, A minimalistic framework for games.
Basket consists on a set of simple abstractions, loaders, and nice features you can use to write game logic atop.

It aims to only implement what is strictly necessary, and it is very opinionated on purpose, yet it allows for custom libraries to implement extra behaviour such as model formats support.

## Creating your first "Application"
The `Application` type is a struct that contains a set of callbacks that will be ran in order for the program to run, setup and function properly.

```c
#include "basket.h"

static int frame(f64 alpha, f64 delta) {
    ren_log("Hello world! FPS: %f", 1.0/delta);
    return 0;
}

int main() {
    eng_set_debug(true);

    return eng_main((Application) {
        .frame = frame
    });
}
```

This above example creates an Application with a callback that executes every frame (about 60hz on most devices, depending on vsync).

It first enables debug mode, which enables debug logging, which we use to print a friendly "hello world" message to the screen, among the FPS counter.

## Fixed timesteps
Basket operates with a fixed timestep, which means that it will try to run a callback N times a second, for things such as simple physics simulations and accurate movement. 

```c
#include "basket.h"

// X:0, Y:0 is center in our coordinate system
int x = 0;
int y = 0;

static int tick(f64 timestep) {
    // Move to bottom right
    x = x + 5;
    y = y + 5;

    return 0;
}

static int frame(f64 alpha, f64 delta) {
    // Render a red rectangle at X, Y
    Color red = { .full = 0xFF0000FF };
    ren_rect(x, y, 16, 16, red);

    return 0;
}

int main() {
    // eng_tickrate(30); in hz, 30hz is the default 

    return eng_main((Application) {
        .frame = frame,
        .tick = tick
    });
}
```

The above code shows up a small red rectangle moving from the middle of the screen to the bottom right.