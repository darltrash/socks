# BASKET

<!-- TOC -->
- [GENERAL](#general)
    - [`Color`](#color)
        - [Example](#example)
    - [`Transform`](#transform)
- [FILESYSTEM.C](#filesystemc)
    - [`const char *fs_read(const char *name, u32 *length)`](#const-char-fs_readconst-char-name-u32-length)
    - [`int sav_identity(const char *identity)`](#int-sav_identityconst-char-identity)
    - [`int sav_store(const char *data, u32 length)`](#int-sav_storeconst-char-data-u32-length)
    - [`char *sav_retrieve(u32 *length)`](#char-sav_retrieveu32-length)
- [MODEL.C](#modelc)
    - [`Vertex`](#vertex)
    - [`VertexAnim` (NYI)](#vertexanim-nyi)
    - [`RenderBox` (NYI)](#renderbox-nyi)
    - [`MeshSlice`](#meshslice)
    - [`Bone` (NYI)](#bone-nyi)
    - [`Animation` (NYI)](#animation-nyi)
    - [`AnimationFrame` (NYI)](#animationframe-nyi)
    - [`AnimationState` (NYI)](#animationstate-nyi)
    - [`Model`](#model)
    - [`int mod_init(Model *model, const char *data)`](#int-mod_initmodel-model-const-char-data)
    - [`void mod_free(Model *model)`](#void-mod_freemodel-model)
- [ENGINE.C](#enginec)
    - [`Application`](#application)
    - [`bool eng_main(Application app, const char *arg0)`](#bool-eng_mainapplication-app-const-char-arg0)
    - [`void eng_close()`](#void-eng_close)
    - [`void eng_tickrate(f64 hz)`](#void-eng_tickratef64-hz)
    - [`void eng_window_size(u16 *w, u16 *h)`](#void-eng_window_sizeu16-w-u16-h)
    - [`void eng_mouse_position(u16 *x, u16 *y)`](#void-eng_mouse_positionu16-x-u16-y)
    - [`bool eng_mouse_down(u8 button)`](#bool-eng_mouse_downu8-button)
    - [`bool eng_is_focused()`](#bool-eng_is_focused)
    - [`void eng_set_debug(bool debug)`](#void-eng_set_debugbool-debug)
    - [`bool eng_is_debug()`](#bool-eng_is_debug)
- [Debug Mode](#debug-mode)
<!-- /TOC -->


## GENERAL
### `Color`
```c
typedef union { 
    struct { u8 a, b, g, r; };
    u8 array[4];
    u32 full;
} Color;
```
An union encoding color as 4 `u8` values (0 to 255) for `RGBA` color.

`COLOR_WHITE` is a macro that encodes pure white using the `Color` type.

#### Example
```c
const Color red = { .full = 0xFF0000FF };
const Color green = { .g = 255 };
const Color blue = { .array = { 0, 0, 255, 255 } };
```

<br>

### `Transform`
```c
typedef struct {
    //  vec3         quat         vec3
    f32 position[3], rotation[4], scale[3];
} Transform;
```
Encodes a transformation.


## FILESYSTEM.C
### `const char *fs_read(const char *name, u32 *length)`
Reads a file of name `name`

It will try to read off the following paths:
- `./package/`
- `./package.bsk`

Sets the `u32` passed by pointer as `length` to the length of the file in bytes, ignored if `NULL`

Returns `NULL` if it was unable to find/read the file.

**Example:**
```c
const char hello_world = fs_read("hello_world.txt", NULL);
printf("hello_world.txt: %s\n", hello_world);

u32 length;
const char image_data = fs_read("atlas.png", &length);

Image image;
img_init(&image, image_data, length);
```

<br>

### `int sav_identity(const char *identity)`
Sets the identity of the game, will try to create a folder in the following paths:
- `C:\Users\<USERNAME>\AppData\Roaming\BASKET\<IDENTITY>`
- `/home/<USERNAME>/.local/share/BASKET/<IDENTITY>`
- `/Users/<USERNAME>/Library/Application Support/<IDENTITY>`

<br>

### `int sav_store(const char *data, u32 length)`
Stores `data` of `length` size into the game's savefile, returns non-zero if failed.

> [!IMPORTANT] 
> Remember to set the identity with [`sav_identity()`](#int-sav_identityconst-char-identity)

<br>

### `char *sav_retrieve(u32 *length)`
Retrieves `data` from a savefile and sets an `u32` to the length of said data.

Returns `NULL` if failed to read savefile, either by the savefile file not existing, some permission error or something else.

// MODEL.C //////////////////////////////////////////////////////

## MODEL.C
### `Vertex`
```c
typedef struct {
    f32 position[3];
    f32 uv[2];
    Color color;
} Vertex;
```
Encodes vertices to be sent to the GPU.

<br>

### `VertexAnim` (NYI)
```c
typedef struct {
    u8 bone[4];
    u8 weight[4];
} VertexAnim;
```
Encodes the vertex part used for animation.

<br>

### `RenderBox` (NYI)
```c
typedef struct {
    f32 min[3];
    f32 max[3];
} RenderBox;
```
Encodes a box used for frustum culling.

<br>

### `MeshSlice`
```c
typedef struct {
    Vertex *data;
    VertexAnim *animation;
    u32 length;
    RenderBox box;
} MeshSlice;
```
Represents a Mesh, a model.

It contains an array of [`Vertex`](#vertex), an optional array of [`VertexAnim`](#vertexanim), their respective size (in amount) and a [`RenderBox`](#renderbox-nyi) for frustum culling (RENDERBOX IS NYI)

Can be taken from a [`Model`](#model)'s mesh property, and thus loaded from [`mod_init`](#mod_init)

<br>

### `Bone` (NYI)
```c
typedef struct {
    char name[64];
    u32 parent;
    Transform transform;
    f32 transform_mat4[16];
} Bone;
```
Encodes an animation bone, NYI!

<br>

### `Animation` (NYI)
```c
typedef struct {
    char *name;
    u32 first, last;
    f32 rate;
    bool loops;
} Animation;
```
Encodes an animation, NYI!

<br>

### `AnimationFrame` (NYI)
```c
typedef Transform* AnimationFrame;
```
Encodes an array of Transform, the frame of an animation. NYI!

<br>

### `AnimationState` (NYI)
```c
typedef struct {
    Bone *bones;
    u32 bone_amount;
    
    Animation *animations;
    u32 animation_amount;

    AnimationFrame *frames, bind_pose, pose;
} AnimationState;
```
Encodes an Animation state, meant to be kept separate from the actual Model.

<br>

### `Model`
```c
typedef struct {
    MeshSlice mesh;
    AnimationState animation;

    char *extra;
} Model;
```
Encodes a model, it contains a [`MeshSlice`](#meshslice) that can be rendered, and an optional `èxtra` value.

Usually this `extra` value encodes a JSON block of assorted content in EXM files.

Also usually treat this type as a transient type, it's only purpose is to account for C's lack of several return values per function.

<br>

### `int mod_init(Model *model, const char *data)`
Loads a Model into `model` from `data`, returns non-zero if failed.

Supports the following model formats:
- Inter Quake Model V2 (`.iqm`)
- Excessive Model (`.exm`)

**Example:**
```c
const char *data = fs_read("my_model.exm", NULL);

Model model;
int ret = mod_init(&model, data);

if (ret)
    print("OK\n");
```

<br>

### `void mod_free(Model *model)`
Unloads a model, internally freeing absolutely all memory inside `model`.

// AUDIO.C //////////////////////////////////////////////////////
    typedef u32 Sound;
    typedef u32 Source;

    Sound aud_load_ogg(const u8 *mem, u32 length, bool spatialize);
    void aud_free(Sound audio);
    Source aud_play(Sound audio);
    void aud_set_position(Sound audio, f32 position[3]);
    void aud_set_velocity(Sound audio, f32 velocity[3]);
    void aud_set_paused(Sound audio, bool paused);
    void aud_set_looping(Sound audio, bool paused);
    void aud_set_pitch(Sound audio, f32 pitch);
    void aud_set_area(Sound audio, f32 distance);
    void aud_set_gain(Sound audio, f32 gain);
    void aud_listener(f32 position[3]);

    #ifdef BASKET_INTERNAL
        int aud_init();
        int aud_byebye();
    #endif


// TEXTURE.C ////////////////////////////////////////////////////
    typedef struct {
        Color *pixels;
        u16 w, h;
    } Image;

    bool img_init(Image *texture, const char *data, u32 length);
    void img_free(Image *texture);


// MAFS.C ///////////////////////////////////////////////////////
    typedef struct {
        f32 left[4];
        f32 right[4];
        f32 top[4];
        f32 bottom[4];
        f32 near[4];
        f32 far[4];
    } Frustum;

    #define IDENTITY_MATRIX { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }

    f32 clamp(f32 v, f32 low, f32 high);
    f32 lerp(f32 a, f32 b, f32 t);

    void mat4_projection(f32 out[16], f32 fovy, f32 aspect, f32 near, f32 far, bool infinite);
    void mat4_ortho(f32 out[16], f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);
    void mat4_lookat(f32 out[16], const f32 eye[3], const f32 at[3], const f32 up[3]);
    void mat4_mul(f32 out[16], const f32 a[16], const f32 b[16]);
    void mat4_invert(f32 out[16], const f32 a[16]);
    void mat4_mulvec(f32 out[3], f32 in[3], f32 m[16]);
    void mat4_from_translation(f32 out[16], const f32 vec[3]);
    void mat4_from_scale(f32 out[16], const f32 vec[3]);
    void mat4_from_angle_axis(f32 out[16], const f32 angle, const f32 axis[3]);
    void mat4_from_euler_angle(f32 out[16], const f32 euler[3]);
    void mat4_from_quaternion(f32 out[16], const f32 quat[4]);
    void mat4_from_transform(f32 out[16], Transform transform);

    f32 vec_dot(const f32 *a, const f32 *b, int len);
    f32 vec_len(const f32 *in, int len);
    void vec_min(f32 *out, const f32 *a, const f32 *b, int len);
    void vec_max(f32 *out, const f32 *a, const f32 *b, int len);
    void vec_add(f32 *out, f32 *a, f32 *b, int len);
    void vec_sub(f32 *out, f32 *a, f32 *b, int len);
    void vec_lerp(f32 *out, f32 *a, f32 *b, f32 t, int len);
    void vec_norm(f32 *out, const f32 *in, int len);
    void vec3_cross(f32 out[3], const f32 a[3], const f32 b[3]) ;

    void frustum_from_mat4(Frustum *f, f32 m[16]);
    bool frustum_vs_aabb(Frustum f, f32 min[3], f32 max[3]);
    bool frustum_vs_sphere(Frustum f, f32 pos[3], f32 radius);
    bool frustum_vs_triangle(Frustum f, f32 a[3], f32 b[3], f32 c[3]);


// RENDERER.C ///////////////////////////////////////////////////
    typedef u8 Texture;

    typedef struct {
        u16 x, y, w, h;
    } TextureSlice;

    typedef struct {
        f32 position[3];
        f32 color[3];
    } Light;

    typedef struct {
        bool disable;
        f32 model[16];
        Color tint;
        MeshSlice mesh;
        TextureSlice texture;
        AnimationState *animation;
    } RenderCall;

    typedef struct {
        f32 position[2];
        f32 scale[2];
        TextureSlice texture;
        Color color;
    } Quad;

    #define DEFAULT_QUAD (Quad){ { 0.0, 0.0 }, { 1.0, 1.0 }, { 0, 0, 0, 0 }, COLOR_WHITE }
    #define LIGHT_AMOUNT 32

    void ren_camera(f32 from[3], f32 to[3]);
    void ren_log(const char *str, ...);
    RenderCall *ren_draw(RenderCall call);
    void ren_light(Light);
    void ren_quad(Quad quad);
    void ren_rect(i32 x, i32 y, u32 w, u32 h, Color color);
    void ren_far(f32 far, Color clear);
    void ren_ambient(Color ambient);
    void ren_snapping(u8 snap);
    void ren_dithering(bool dither);
    void ren_size(u16 *w, u16 *h);
    void ren_mouse_position(i16 *x, i16 *y);
    void ren_videomode(u16 w, u16 h, bool force_ratio);

    Texture ren_tex_load(const char *data, u32 length);
    Texture ren_tex_load_custom(Image image);
    void ren_tex_free(Texture id);
    void ren_tex_bind(Texture main, Texture lumos);

    #ifdef BASKET_INTERNAL
        bool ren_init(SDL_Window *window);
        int ren_frame();
        void ren_byebye();
    #endif


// INPUT.C //////////////////////////////////////////////////////
    enum {
        INP_NONE = 0,

        INP_UP,
        INP_DOWN,
        INP_LEFT,
        INP_RIGHT,

        INP_JUMP,
        INP_ATTACK,
        INP_MENU,

        INP_MAX
    };

    const char *inp_text();
    void inp_clear();
    u32 inp_button(u8 button);
    
    bool inp_register_scancode(const char *scancode, u8 button);
    bool inp_register_keycode(const char *keycode, u8);
    const char *inp_get_key(u8 button);

    #ifdef BASKET_INTERNAL
        void inp_setup();
        void inp_event(SDL_Event event);
        bool inp_update(f64 timestep);
    #endif


## ENGINE.C
### `Application`
This struct contains a handful of Callbacks to be called in order, they all return a status code which, if non-zero, will cause the program to fail.

```C
typedef struct {
    int (*init)  ();
    int (*frame) (f64 alpha, f64 delta);
    int (*tick)  (f64 timestep);
    int (*close) (void);
} Application; 
```

- `init()`: Will be called once the engine is set up but not yet processing any frames.
- `frame(f64 alpha, f64 delta)`: Will be called every frame, with alpha representing the interpolation time between two ticks, and delta representing how much seconds did the last frame took to render (smoothed)
- `tick(f64 timestep)`: Will be called every N hertz (or `timestep` seconds), defined by [`eng_tickrate()`](#void-eng_tickratef64-hz), default is 30hz
- `close()`: Executes once the Application quits, either by [`eng_close()`](#void-eng_close) or by an error return value on any of the callbacks (except itself, of course).

> [!IMPORTANT] 
> Event polling happens at the same timestep as tick() is run, so don't go too nuts on it or else events will be VERY delayed. 

<br>

### `bool eng_main(Application app, const char *arg0)`
Runs an [`Application`](#application), and returns the status code.
Zero is okay, non-zero is error.

> 


**Example:**
```c
static int tick(f64 tickrate) {
    printf("Hello world!\n");
}

static Application my_game = {
    .frame = frame
}

int main(int argc, char *argv[]) {
    return eng_main(my_game, argv[0]);
}
```

<br>

### `void eng_close()`
Forces the engine to stop after frame is finished, once it's called it cannot be undone, meant to be used as a way to signal that you wanted to close the Application because it is intended behaviour and not an error.

If it is because of an error, you might want to take a look at [`Application`](#application)'s use of return values on callbacks instead.

<br>

### `void eng_tickrate(f64 hz)`
Sets the tickrate (in hertz) at which the [`Application.tick()`](#application) is ran, and at which rate to poll the events.

<br>

### `void eng_window_size(u16 *w, u16 *h)`
Sets two `u16` integers to the width and height of the window respectively.

If any pointer is `NULL`, it will be ignored.

**Example:**
```c
u16 width, height;
eng_window_size(&width, &height);

printf("Width:  %u\n", width);
printf("Height: %u\n", height);
```

<br>

### `void eng_mouse_position(u16 *x, u16 *y)`
Sets two `u16` integers to the X and Y of the cursor respectively.

If any pointer is `NULL`, it will be ignored.

**Example:**
```c
u16 mx, my;
eng_mouse_position(&mx, &my);

printf("Mouse X: %u\n", mx);
printf("Mouse Y: %u\n", my);
```
<br>

### `bool eng_mouse_down(u8 button)`
Returns true whether mouse button is pressed in the current tick.

<br>

### `bool eng_is_focused()`
Returns true if Application window is focused or not.

<br>

### `void eng_set_debug(bool debug)`
Enables or disables [Debug Mode](#debug-mode).

<br>

### `bool eng_is_debug()`
Returns true if [Debug Mode](#debug-mode) is enabled

## Debug Mode
Debug mode enables a set of useful debug functions and utilities, such as:
- Enabling [`ren_log()`](#ren-log)
- Enabling internal TinyFX OpenGL logs

It can be enabled by [`eng_set_debug()`](#void-eng_set_debugbool-debug) and retrieved by [`eng_is_debug()`](#bool-eng_is_debug).