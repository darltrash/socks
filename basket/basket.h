#include <stdint.h>
#include <stdbool.h>

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
 
/*
    Basket design rules/style:
    - Assume people will build their software around Basket, as a framework
    - Null output (in case of arrays/pointers) or non-zero (true) equals error.
    - snake_case for variables
    - camelCase for functions
    - PascalCase for types (except for number types)
    - (const) char* for strings, not u8*
    - Share the same header for programs and internals
    - Default to 0, False, NULL

    TODO:
    - Add several passes for rendering with different textures, setups, etc.
    - Add full featured system API, with self-extractable features
    - Finish Audio API, make it comfier to use
    - Documentation
*/


// General //////////////////////////////////////////////////////
    typedef union { 
        struct { u8 a, b, g, r; };
        u8 array[4];
        u32 full;
    } Color;

    typedef struct {
        //  vec3         quat         vec3
        f32 position[3], rotation[4], scale[3];
    } Transform;

    typedef struct {
        f32 position[3];
        f32 uv[2];
        Color color;
    } Vertex;

    typedef union{ 
        struct {
            Vertex a, b, c;
        };
        Vertex arr[3];
    } Triangle;

    typedef struct {
        f32 min[3];
        f32 max[3];
    } Box;

    typedef struct {
        u32 offset, length;
    } Range;

    #define COLOR_WHITE (Color){ .full=0xffffffff }

    #ifdef BASKET_INTERNAL
        #include <SDL2/SDL.h>

        // ... oh, windows.
        #ifdef _WIN32
            #undef near
            #undef far
        #endif

        #define  alloc(type,n) (calloc(n,  sizeof(type)))
        #define falloc(type,n) (malloc(n * sizeof(type)))
        #define min(a,b) (((a)<(b))?(a):(b))
        #define max(a,b) (((a)>(b))?(a):(b))
    #endif

    enum  {
        BSKT_UNKNOWN = 0xBA5, // also known as min

        BSKT_FS_COULDNT_FIND_PACKAGE,

        BSKT_SAV_COULDNT_SAVE,

        BSKT_MOD_UNRECOGNIZED_FORMAT,
        
        BSKT_AUD_,

        BSKT_IMG_COULDNT_LOAD_IMAGE,

        BSKT_REN_COULDNT_INIT,
        BSKT_REN_COULDNT_RENDER_FRAME
    };


// FILESYSTEM.C /////////////////////////////////////////////////
    char *fs_read(const char *name, u32 *length);

    int sav_identity(const char *identity);
    int sav_store(const char *data, u32 length);
    char *sav_retrieve(u32 *length);

    #ifdef BASKET_INTERNAL
        int fs_init(const char *self);
    #endif


// MODEL.C //////////////////////////////////////////////////////
    typedef struct {
        u8 bone[4];
        u8 weight[4];
    } VertexAnim;

    typedef struct {
        Vertex *data;
        VertexAnim *animation; // Can be NULL
        u32 length;
        Box box;
    } MeshSlice;

    typedef struct {
        char name[64];
        u32 parent;
        Transform transform;
        f32 transform_mat4[16];
    } Bone;

    typedef struct {
        char *name;
        u32 first, last;
        f32 rate;
        bool loops;
    } Animation;

    typedef Transform* AnimationFrame;

    typedef struct {
        Bone *bones;
        u32 bone_amount;
        
        Animation *animations;
        u32 animation_amount;

        AnimationFrame *frames, bind_pose, pose;
    } AnimationState;

    typedef struct {
        char *name;
        Range range;
    } SubMesh;

    typedef struct {
        MeshSlice mesh;
        AnimationState animation;

        SubMesh *submeshes;
        u32 submesh_amount;

        char *extra;
    } Model;

    int mod_init(Model *model, const char *data);
    void mod_free(Model *model);


// AUDIO.C //////////////////////////////////////////////////////
    typedef u32 Sound;  // Raw sound
    typedef u32 Source; // Sound source (spatial)

    int aud_load_ogg(Sound *sound, const u8 *mem, u32 length, bool spatialize);
    int aud_init_source(Source *source, Sound audio);
    void aud_free(Source audio);
    void aud_play(Source audio);
    void aud_set_position(Source audio, f32 position[3]);
    void aud_set_velocity(Source audio, f32 velocity[3]);
    void aud_set_paused(Source audio, bool paused);
    void aud_set_looping(Source audio, bool paused);
    void aud_set_pitch(Source audio, f32 pitch);
    void aud_set_area(Source audio, f32 distance);
    void aud_set_gain(Source audio, f32 gain);

    void aud_listener(f32 position[3]);
    void aud_orientation(f32 towards[3], f32 up[3]);

    void aud_gain(f32 volume);
    void aud_pause(bool pause);

    #ifdef BASKET_INTERNAL
        int aud_init();
        int aud_byebye();
    #endif


// TEXTURE.C ////////////////////////////////////////////////////
    typedef struct {
        Color *pixels;
        u16 w, h;
    } Image;

    int img_init(Image *image, const char *data, u32 length);
    void img_free(Image *image);


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

    #define QUICK_SCALE_MATRIX(x, y, z) { x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1 }
    #define QUICK_TRANSLATION_MATRIX(x, y, z) { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1 }

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
    void vec_mul(f32 *out, f32 *a, f32 *b, int len);
    void vec_scale(f32 *out, f32 *a, f32 b, int len);
    void vec_lerp(f32 *out, f32 *a, f32 *b, f32 t, int len);
    void vec_norm(f32 *out, const f32 *in, int len);
    void vec3_cross(f32 out[3], const f32 a[3], const f32 b[3]) ;

    void frustum_from_mat4(Frustum *f, f32 m[16]);
    bool frustum_vs_aabb(Frustum f, f32 min[3], f32 max[3]);
    bool frustum_vs_sphere(Frustum f, f32 pos[3], f32 radius);
    bool frustum_vs_triangle(Frustum f, f32 a[3], f32 b[3], f32 c[3]);


// POOL.C ///////////////////////////////////////////////////////

typedef struct VertexPool VertexPool;

struct VertexPool {
    VertexPool* left;
    VertexPool* right;

    Box bounds;
    Triangle* triangles;
    u32 triangle_count;
};

int pool_init(VertexPool* pool, Triangle* triangles, u32 count);
void pool_free(VertexPool* node);


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
        Range range;
    } RenderCall;

    typedef struct {
        f32 position[2];
        f32 scale[2];
        TextureSlice texture;
        Color color;
    } Quad;

    #define DEFAULT_QUAD (Quad){ { 0.0, 0.0 }, { 1.0, 1.0 }, { 0, 0, 0, 0 }, COLOR_WHITE }
    #define LIGHT_AMOUNT 32

    void ren_log(const char *str, ...);
    
    RenderCall *ren_render(RenderCall call);
    void ren_light(Light);
    
    RenderCall *ren_draw(RenderCall call);
    void ren_quad(Quad quad);
    void ren_rect(i32 x, i32 y, u32 w, u32 h, Color color);
    
    void ren_camera(f32 from[3], f32 to[3], f32 up[3]);
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
        int ren_init(SDL_Window *window);
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
        int inp_init();
        void inp_byebye();
        void inp_event(SDL_Event event);
        bool inp_update(f64 timestep);
    #endif


// ENGINE.C
    typedef struct {
        int (*init)  ();
        int (*frame) (f64 alpha, f64 delta);
        int (*tick)  (f64 timestep);
        int (*close) (int ret);
    } Application; 


    int  eng_main(Application app, const char *arg0);
    void eng_close(); // Will close at the end of the frame
    void eng_halt(const char* str, ...); // Will force the game to close
    void eng_tickrate(f64 hz);

    void eng_window_size(u16 *w, u16 *h); // TODO: This shit is not future proof.
    void eng_mouse_position(u16 *x, u16 *y);
    bool eng_mouse_down(u8 button);

    bool eng_is_focused();
    void eng_set_debug(bool debug);
    bool eng_is_debug();

    const char *eng_error_string(int error);
