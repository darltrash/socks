#pragma once

// big header boy, it glues everything together.

#include <SDL2/SDL_keycode.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// ... oh, windows.
#ifdef _WIN32
    #undef near
    #undef far
#endif


typedef double f64;
typedef float f32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

#define  alloc(type,n) (calloc(n, sizeof(type)))
#define falloc(type,n) (malloc(n * sizeof(type)))
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

// General
typedef union { 
    struct { u8 a, b, g, r; };
    u8 array[4];
    u32 full;
} Color;

#define COLOR_WHITE (Color){ .full=0xffffffff }


// MAFS.C
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
void mat4_from_translation(f32 out[16], f32 vec[3]);
void mat4_from_scale(f32 out[16], f32 vec[3]);
void mat4_from_angle_axis(f32 out[16], f32 angle, f32 axis[3]);
void mat4_from_euler_angle(f32 out[16], f32 euler[3]);
f32 vec_dot(const f32 *a, const f32 *b, int len);
f32 vec_len(const f32 *in, int len);
void vec_min(f32 *out, const f32 *a, const f32 *b, int len);
void vec_max(f32 *out, const f32 *a, const f32 *b, int len);
void vec_add(f32 *out, f32 *a, f32 *b, int len);
void vec_sub(f32 *out, f32 *a, f32 *b, int len);
void vec_lerp(f32 *out, f32 *a, f32 *b, f32 t, int len);
void vec_norm(f32 *out, const f32 *in, int len);
void vec3_cross(f32 *out, const f32 *a, const f32 *b);
void frustum_from_mat4(Frustum *f, f32 m[16]);
bool frustum_vs_aabb(Frustum f, f32 min[3], f32 max[3]);
bool frustum_vs_sphere(Frustum f, f32 pos[3], f32 radius);
bool frustum_vs_triangle(Frustum f, f32 a[3], f32 b[3], f32 c[3]);


// RENDERER.C
typedef struct {
    f32 position[3];
    f32 uv[2];
    Color color;
} Vertex;

typedef struct {
    f32 min[3];
    f32 max[3];
} RenderBox;

typedef struct {
    Vertex *data;
    u32 length;
    RenderBox box;
} MeshSlice;

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
} RenderCall;

typedef struct {
    f32 position[2];
    f32 scale[2];
    TextureSlice texture;
    Color color;
} Quad;

#define DEFAULT_QUAD (Quad){ { 0.0, 0.0 }, { 1.0, 1.0 }, { 0, 0, 0, 0 }, COLOR_WHITE }

bool ren_init(SDL_Window *window);
int ren_frame();
void ren_resize(u16, u16);
void ren_camera(f32 from[3], f32 to[3]);
void ren_log(const char *str, ...);
RenderCall *ren_draw(RenderCall);
void ren_light(Light);
void ren_quad(Quad quad);
void ren_rect(i32 x, i32 y, u32 w, u32 h, Color color);
void ren_far(f32 far, Color clear);
void ren_ambient(Color ambient);
void ren_snapping(u8 snap);
void ren_dithering(bool dither);
void eng_size(u16 *w, u16 *h);
void ren_byebye();


// FILESYSTEM.C
const char *fs_read(const char *name, u32 *length);


// MODEL.C
typedef struct {
    MeshSlice mesh;
    char *extra;
} Map;

Map mod_load(const char *data);


// INPUT.C
enum {
    INP_NONE = 0,

    INP_UP,
    INP_DOWN,
    INP_LEFT,
    INP_RIGHT,

    INP_JUMP,
    INP_QUICK,
    INP_MENU,

    INP_MAX
};

typedef struct {
    u8 binding;
    SDL_Scancode code;
} InputBinding;

const char *inp_text();
void inp_setup();
void inp_clear();
void inp_event(SDL_Event event);
bool inp_update(f64 delta);
u32 inp_button(u8 button);
bool inp_register_scancode(const char *scancode, u8 button);
bool inp_register_keycode(const char *keycode, u8);
const char *inp_get_key(u8 button);


// ENGINE.C
typedef struct {
    bool (*init)  (void);
    bool (*frame) (f64 alpha, f64 delta, bool focused);
    bool (*tick)  (void);
} Application; 

bool eng_main(Application);
void eng_close(); // Will close at the end of the frame
void eng_window_size(u16 *w, u16 *h); // TODO: This shit is not future proof.
void eng_mouse_position(u16 *x, u16 *y);
bool eng_mouse_down(u8 button);


// SAVE.C
int sav_identity(const char *identity);
int sav_store(const char *data, size_t length);
char *sav_retrieve(u32 *length);