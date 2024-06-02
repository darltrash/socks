// implement renderer, the single most advanced piece of code in src/
// do not be afraid, embrace it.

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BASKET_INTERNAL
#include "basket.h"
#include "vec.h"
#include "tinyfx.h"

#include "shaders.h"

#define REN_ATLAS_WIDTH 512
#define REN_ATLAS_HEIGHT 512

static int width, height;

typedef vec_t(Vertex) VertexVec;
typedef vec_t(Quad) QuadVec;
typedef vec_t(RenderCall) CallVec;
typedef vec_t(Light) LightVec;

static vec_char_t logs;
static CallVec calls;
static QuadVec quads; 
static LightVec lights;

static tfx_uniform proj_uniform;
static tfx_uniform image_uniform;
static tfx_uniform lumos_uniform;
static tfx_uniform res_uniform;
static tfx_uniform real_res_uniform;
static tfx_uniform clear_uniform;
static tfx_uniform ambient_uniform;
static tfx_uniform target_uniform;
static tfx_uniform far_uniform;
static tfx_uniform snap_uniform;
static tfx_uniform dither_uniform;
static tfx_uniform scale_uniform;
static tfx_uniform lposition_uniform;
static tfx_uniform lcolor_uniform;
static tfx_uniform lamount_uniform;

static tfx_program program;
static tfx_program out_program;
static tfx_program quad_program;

static Color clear_color = { .full = 0x000000FF };
static Color ambient = { .full=0xFFFFFFFF };

static f32 far = 15.0;
static int snapping = 0;
static bool dithering = true;
static bool compat_mode = true;
static bool resize; 
static bool was_debug = false;

static tfx_vertex_format vertex_format;
static f32 view_matrix[16] = IDENTITY_MATRIX;

u16 target_w, target_h;
bool enable_fill;
f32 scale = 1.0;

SDL_Window *window;

static void tfx_debug_thingy(const char *str, tfx_severity severity) {
    if (!eng_is_debug()) 
        return;

    printf("tfx ");
    
    switch (severity) {
        case TFX_SEVERITY_INFO: {
            printf("info: ");
            break;
        }

        case TFX_SEVERITY_WARNING: {
            printf("warn: ");
            break;
        }

        case TFX_SEVERITY_ERROR: {
            printf("ERROR: ");
            break;
        }

        case TFX_SEVERITY_FATAL: {
            printf("FATAL: ");
            break;
        }
    }

    printf("%s\n", str);
}

void ren_videomode(u16 w, u16 h, bool fill) {
    target_w = w;
    target_h = h;
    enable_fill = fill;
    SDL_SetWindowMinimumSize(window, w, h);
    resize = true;
}

#define TEXTURE_AMOUNT 128
static tfx_texture textures[TEXTURE_AMOUNT];
static tfx_texture texture_none;
tfx_texture texture_main;
tfx_texture texture_lumos;

u8 ren_tex_load(const char *data, u32 length) {
    Image tex;
    if (img_load(data, length, &tex))
        return 0;

    u8 id = ren_tex_load_custom(tex);
    img_free(&tex);

    return id;
}

u8 ren_tex_load_custom(Image img) {
    for (int i = 0; i < TEXTURE_AMOUNT; i++) {
        if (!textures[i].gl_count) {
            textures[i] = tfx_texture_new(
                img.w, img.h, 1, img.pixels, 
                TFX_FORMAT_RGBA8, TFX_TEXTURE_FILTER_POINT
            );
            return i+1;
        }
    }

    return 0;
}

void ren_tex_free(u8 id) {
    if (!id) return;

    id -= 1;

    if (!textures[id].gl_count)
        return;

    tfx_texture_free(&textures[id]);
    textures[id].gl_count = 0;
}

void ren_tex_bind(u8 main, u8 lumos) {
    texture_main = texture_none;
    texture_lumos = texture_none;

    if (main && textures[main-1].gl_count)
        texture_main = textures[main-1];

    if (lumos && textures[lumos-1].gl_count)
        texture_lumos = textures[lumos-1];
}

static tfx_program shader(const char *data, const char *attribs[]) {
    // lazy
    u32 final_length = strlen(data) + strlen(library_glsl) + 32;
    char *final_source = malloc(final_length);

    if (compat_mode) 
        snprintf(final_source, final_length, "#define COMPAT_MODE 1\n%s\n%s\n", library_glsl, data);
    else
        snprintf(final_source, final_length, "%s\n%s\n", library_glsl, data);

    tfx_program program = tfx_program_new (
        final_source, 
        final_source, 
        attribs, -1
    );

    return program;
}


bool ren_init(SDL_Window *_window) {
    printf("setting up renderer.\n");

    window = _window;

    i8 vsync = 1;
    if (SDL_GL_ExtensionSupported("EXT_swap_control_tear"))
        vsync = -1;

    if (getenv("BK_NO_VSYNC"))
        vsync = 0;

    SDL_GL_SetSwapInterval(vsync);

	SDL_GLContext *context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);

    ren_videomode(350, 350, true);

    // setup opengl bullshit
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    int version = 33;

    #ifdef _WIN32
        // gles2 is not well supported on Windows
        compat_mode = false;
    #endif

    if (compat_mode) {
        version = 20;
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    }

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    // setup tinyfx stuff
    tfx_platform_data pd;
	pd.use_gles = compat_mode;
	pd.context_version = version;
	pd.gl_get_proc_address = SDL_GL_GetProcAddress;
    pd.info_log = tfx_debug_thingy;

	tfx_set_platform_data(pd);
    tfx_reset(1, 1, 0);

    Color transparent = {0, 0, 0, 0};
    texture_none = tfx_texture_new(1, 1, 1, &transparent, TFX_FORMAT_RGBA8, 0);

    ren_tex_bind(0, 0);
    
    const char *attribs[] = {
        "vx_position",
        "vx_uv",
        "vx_color",
        NULL
    };

    program      = shader(shader_glsl, attribs);
    out_program  = shader(output_glsl, attribs);
    quad_program = shader(quad_glsl,   attribs);

	vertex_format = tfx_vertex_format_start();
	tfx_vertex_format_add(&vertex_format, 0, 3, false, TFX_TYPE_FLOAT); // Position
	tfx_vertex_format_add(&vertex_format, 1, 2, false, TFX_TYPE_FLOAT); // UV
	tfx_vertex_format_add(&vertex_format, 2, 4, false, TFX_TYPE_UBYTE); // Color
	tfx_vertex_format_end(&vertex_format);

    proj_uniform     = tfx_uniform_new("projection",      TFX_UNIFORM_MAT4, 1);
    image_uniform    = tfx_uniform_new("image",           TFX_UNIFORM_INT,  1);
    lumos_uniform    = tfx_uniform_new("lumos",           TFX_UNIFORM_INT,  1);
    res_uniform      = tfx_uniform_new("resolution",      TFX_UNIFORM_VEC2, 1);
    real_res_uniform = tfx_uniform_new("real_resolution", TFX_UNIFORM_VEC2, 1);

    clear_uniform    = tfx_uniform_new("clear",      TFX_UNIFORM_VEC4,  1);
    ambient_uniform  = tfx_uniform_new("ambient",    TFX_UNIFORM_VEC3,  1);
    target_uniform   = tfx_uniform_new("target",     TFX_UNIFORM_VEC3,  1);
    far_uniform      = tfx_uniform_new("far",        TFX_UNIFORM_FLOAT, 1);
    snap_uniform     = tfx_uniform_new("snapping",   TFX_UNIFORM_INT,   1);
    dither_uniform   = tfx_uniform_new("dither",     TFX_UNIFORM_INT,   1);
    scale_uniform    = tfx_uniform_new("scale",      TFX_UNIFORM_INT,   1);

    lposition_uniform = tfx_uniform_new("light_positions", TFX_UNIFORM_VEC3, 16);
    lcolor_uniform    = tfx_uniform_new("light_colors",    TFX_UNIFORM_VEC3, 16);
    lamount_uniform   = tfx_uniform_new("light_amount",    TFX_UNIFORM_INT,  1);

    vec_init(&logs);
    vec_init(&calls);
    vec_init(&quads);
    vec_init(&lights);

    return false;
}

f32 camera_target[3] = { 0.0f, 0.0f, 0.0f };
void ren_camera(f32 from[3], f32 to[3]) {
    const f32 up[3] = { 0.0f, 0.0f, 1.0f };
    
    mat4_lookat(view_matrix, from, to, up);
    mat4_mulvec(camera_target, to, view_matrix);
}

RenderCall *ren_draw(RenderCall call) {
    vec_push(&calls, call);
    return &calls.data[calls.length];
}

typedef union{ 
    struct {
        Vertex a, b, c;
    };
    Vertex arr[3];
} Triangle;

void ren_quad(Quad quad) {
//    f32 w = quad.texture.w * quad.scale[0];
//    f32 h = quad.texture.h * quad.scale[1];
//
//    if (!aabb(quad.position[0], quad.position[1], w, h, -150, -150, 300, 300))
//        return;

    vec_push(&quads, quad);
}

void ren_light(Light light) {
    vec_push(&lights, light);
}


void ren_rect(i32 x, i32 y, u32 w, u32 h, Color color) {
    ren_quad((Quad) {
        .position = { (f32)x, (f32)y },
        .texture = { 0, 0, 1, 1 },
        .color = color,
        .scale = { w, h }
    });
}

void ren_log(const char *str, ...) {
    if (!eng_is_debug()) 
        return;

    static char buffer[256];

    va_list argptr;
    va_start(argptr, str);
    vsnprintf(buffer, 256, str, argptr);
    va_end(argptr);

    u32 len = strlen(buffer);
    buffer[len] = '\n';

    vec_pusharr(&logs, buffer, len+1);
}

void ren_far(f32 f, Color _clear_color) {
    far = f;
    clear_color = _clear_color;
}

void ren_ambient(Color _ambient) {
    ambient = _ambient;
}

void ren_snapping(u8 _snapping) {
    snapping = _snapping;
}

void ren_dithering(bool _dithering) {
    dithering = _dithering;
}

static f32 resolution[2];

void ren_size(u16 *w, u16 *h) {
    if (enable_fill) {
        *w = (u16)ceilf((f32)width /scale);
        *h = (u16)ceilf((f32)height/scale);
        return;
    }

    *w = (u16)ceilf((f32)target_w/scale);
    *h = (u16)ceilf((f32)target_h/scale);
}

int ren_frame() {
    static tfx_canvas canvas;
    static Frustum frustum;

    static VertexVec scene_triangles;

    static f32 proj_matrix[16];

    int curr_width, curr_height;
    SDL_GL_GetDrawableSize(window, &curr_width, &curr_height);

    if (curr_width != width || curr_height != height)
        resize = true;

    bool is_debug = eng_is_debug();
    if (is_debug != was_debug) {
        resize = true;

        was_debug = is_debug;
    }

    if (resize) {
        resize = false;

        width = curr_width;
        height = curr_height;

        tfx_reset_flags flags = TFX_RESET_NONE;

        if (eng_is_debug()) {
            flags = 0
                | TFX_RESET_DEBUG_OVERLAY 
                | TFX_RESET_DEBUG_OVERLAY_STATS
                | TFX_RESET_REPORT_GPU_TIMINGS;
        }

        tfx_reset(width, height, flags);

        scale = max(1.0, floorf(min(width / target_w, height / target_h)));

        resolution[0] = target_w;
        resolution[1] = target_h;
        
        if (enable_fill) {
            resolution[0] = (f32)(width )/scale;
            resolution[1] = (f32)(height)/scale;
        }

        if (canvas.gl_fbo[0] > 0)
            tfx_canvas_free(&canvas);

        canvas = tfx_canvas_new(
            resolution[0], resolution[1], 
            TFX_FORMAT_RGBA8_D24, 
            TFX_TEXTURE_FILTER_POINT
        );

        tfx_set_uniform(&res_uniform, resolution, 1);

        f32 real_res[2] = { (f32)(width), (f32)(height) };
        tfx_set_uniform(&real_res_uniform, real_res, 1);

        int pixelsize = scale;
        tfx_set_uniform_int(&scale_uniform, &pixelsize, 1);

        if (!scene_triangles.data)
            vec_init(&scene_triangles);
    }

    const f32 aspect = resolution[0] / resolution[1];

    mat4_projection(proj_matrix, 80, aspect, 0.001f, far+6.0, false);
    tfx_set_uniform(&proj_uniform, proj_matrix, 1);
    frustum_from_mat4(&frustum, proj_matrix);

    f32 clear_f[4] = {
        (f32)(clear_color.r) / 255.0,
        (f32)(clear_color.g) / 255.0,
        (f32)(clear_color.b) / 255.0,
        (f32)(clear_color.a) / 255.0
    };

    f32 ambient_f[3] = {
        (f32)(ambient.r) / 255.0,
        (f32)(ambient.g) / 255.0,
        (f32)(ambient.b) / 255.0
    };

    tfx_set_uniform(&target_uniform, camera_target, 1);
    tfx_set_uniform(&clear_uniform, clear_f, 1);
    tfx_set_uniform(&ambient_uniform, ambient_f, 1);
    tfx_set_uniform(&far_uniform, &far, 1);
    tfx_set_uniform_int(&snap_uniform, &snapping, 1);

    // HANDLE LIGHTING
    static f32 m[16];

    int real_index = 0;
    static f32 light_positions[LIGHT_AMOUNT*3];
    static f32 light_colors[LIGHT_AMOUNT*3];

    for (int i = 0; i < lights.length; i++) {
        Light light = lights.data[i];

        f32 area = vec_len(light.color, 3);

        f32 *color = &light_colors[real_index * 3];
        f32 *position = &light_positions[real_index * 3];

        mat4_mulvec(position, light.position, view_matrix);

        if (frustum_vs_sphere(frustum, position, area * 2)) {
            memcpy(color, light.color, sizeof(light.color));

            if (real_index++ > LIGHT_AMOUNT)
                break;
        }
    }

    vec_clear(&lights);

    if (real_index) {
        tfx_set_uniform(&lposition_uniform, light_positions, -1);
        tfx_set_uniform(&lcolor_uniform, light_colors, -1);
    }

    tfx_set_uniform_int(&lamount_uniform, &real_index, -1);
    tfx_set_uniform_int(&dither_uniform, (int *)&dithering, -1);

    tfx_set_state(TFX_STATE_RGB_WRITE | TFX_STATE_DEPTH_WRITE);

    // RENDER TRIANGLES
    const u8 view = 1;
    tfx_view_set_clear_depth(view, 1.0);
    tfx_view_set_depth_test(view, TFX_DEPTH_TEST_LT);
    tfx_view_set_clear_color(view, clear_color.full);
	tfx_view_set_name(view, "the main pass");
    tfx_view_set_canvas(view, &canvas, 0);

    for (u32 i = 0; i < calls.length; i++) {
        RenderCall call = calls.data[i];
        if (call.disable) continue;
        if (call.tint.a == 0) continue;

        if (call.texture.w == 0) {
            call.texture.w = texture_main.width;
            call.texture.h = texture_main.height;
        }

        mat4_mul(m, call.model, view_matrix);

        //printf("sexo? %i\n", call.mesh.length);

        for (u32 a = 0; a < call.mesh.length; a+=3) {
            Triangle tri;

            for (u32 b = 0; b < 3; b++) {
                Vertex vertex = call.mesh.data[a+b];
                Vertex *copy = &tri.arr[b];

                #define _CCM(a,b) (u8) (((unsigned)a * (unsigned)b + 255u) >> 8)
                #define _CKW(a) ((f32)(a) / (f32)texture_main.width)
                #define _CKH(a) ((f32)(a) / (f32)texture_main.height)
                //#define _CCM(fc,oc) (fc)

                copy->uv[0] = _CKW(call.texture.w) * vertex.uv[0] + _CKW(call.texture.x);
                copy->uv[1] = _CKH(call.texture.h) * vertex.uv[1] + _CKH(call.texture.y);

                u8 fr = _CCM(call.tint.r, vertex.color.r);
                u8 fg = _CCM(call.tint.g, vertex.color.g);
                u8 fb = _CCM(call.tint.b, vertex.color.b);
                u8 fa = _CCM(call.tint.a, vertex.color.a);

                copy->color = (Color) { fr, fg, fb, fa };

                //printf("fa %i\n", fa);

                mat4_mulvec(copy->position, vertex.position, m);
            }

            if (frustum_vs_triangle(frustum, tri.a.position, tri.b.position, tri.c.position))
                vec_pusharr(&scene_triangles, tri.arr, 3);
        }
    }
    vec_clear(&calls);

    Triangle *t_list = (Triangle *)scene_triangles.data;
    const u32 t_amount = scene_triangles.length/3;

    tfx_transient_buffer buffer = tfx_transient_buffer_new(&vertex_format, t_amount*3);
    memcpy(buffer.data, scene_triangles.data, t_amount*sizeof(Triangle));

    vec_clear(&scene_triangles);

    ren_log("\n// RENDERER //////");
    ren_log("TRIANGLES:  %i", t_amount);
    ren_log("RESOLUTION: %ix%i", width, height);
    ren_log("QUADS:      %i", quads.length);
    ren_log("LIGHTS:     %i", real_index);

    const u32 s = 
            (quads.length * 2 * sizeof(Triangle)) +
            (t_amount * sizeof(Triangle));

    const f32 size = (float)(s) / 1024.0f;
    ren_log("GPU UPLOADS: (%.3gkb)", size);

    vec_push(&logs, 0);

    tfx_debug_print(8, 0, 9, 5, logs.data);


    vec_clear(&logs);

    tfx_set_transient_buffer(buffer);
    tfx_set_texture(&image_uniform, &texture_main, 0);
    tfx_set_texture(&lumos_uniform, &texture_lumos, 1);
    tfx_submit(view, program, false);
    

    // RENDER QUADS
    const u8 ui = 3;
    tfx_set_state(TFX_STATE_RGB_WRITE);
    tfx_view_set_name(ui, "the quad pass");
    tfx_view_set_canvas(ui, &canvas, 0);
    tfx_set_texture(&image_uniform, &texture_main, 0);
    tfx_transient_buffer quad_buffer = tfx_transient_buffer_new(&vertex_format, quads.length*6);

    const f32 w = resolution[0] / 2.f;
    const f32 h = resolution[1] / 2.f;

    mat4_ortho(m, -w, w, -h, h, 0.01f, 100.f);

    Vertex *quad_vertices = quad_buffer.data;

    for (u32 i = 0; i < quads.length; i++) {
        Quad q = quads.data[i];

        const f32 tsx = (q.texture.x) / (f32)texture_main.width;
        const f32 tsy = (q.texture.y) / (f32)texture_main.height;
        const f32 tcx = (q.texture.x + q.texture.w) / (f32)texture_main.width;
        const f32 tcy = (q.texture.y + q.texture.h) / (f32)texture_main.height;

        const f32 qsx = q.position[0];
        const f32 qsy = q.position[1];
        const f32 qcx = q.position[0] + (q.texture.w * q.scale[0]);
        const f32 qcy = q.position[1] + (q.texture.h * q.scale[1]);

        #define vertex(e, x, y, u, v) {\
            Vertex tmp = (Vertex) { \
                { x, y, 0.f }, { u, v }, \
                { q.color.r, q.color.g, q.color.b, q.color.a } \
            }; \
            memcpy(quad_vertices[(i*6)+e].position, &tmp, sizeof(Vertex)); \
            mat4_mulvec(quad_vertices[(i*6)+e].position, tmp.position, m); \
        }

        vertex(0, qsx, qsy, tsx, tsy)
        vertex(1, qsx, qcy, tsx, tcy)
        vertex(2, qcx, qsy, tcx, tsy)

        vertex(3, qcx, qcy, tcx, tcy)
        vertex(4, qcx, qsy, tcx, tsy)
        vertex(5, qsx, qcy, tsx, tcy)

    }
    vec_clear(&quads);

    tfx_set_transient_buffer(quad_buffer);
    tfx_submit(ui, quad_program, false);


    // RENDER OUTPUT
    const u8 post = 4;
    tfx_set_state(TFX_STATE_RGB_WRITE);
    tfx_view_set_depth_test(post, TFX_DEPTH_TEST_NONE);
	tfx_view_set_name(post, "the output pass");

    tfx_transient_buffer flat_quad = tfx_transient_buffer_new(&vertex_format, 6);
    Vertex quad[6] = {
        {{ -1.0, -1.0, 0.0 }},
        {{ -1.0,  1.0, 0.0 }},
        {{  1.0, -1.0, 0.0 }},

        {{  1.0,  1.0, 0.0 }},
        {{  1.0, -1.0, 0.0 }},
        {{ -1.0,  1.0, 0.0 }},
    };
    memcpy(flat_quad.data, quad, sizeof(quad));
    tfx_set_transient_buffer(flat_quad);

    tfx_texture tex = tfx_get_texture(&canvas, 0);
    tfx_set_texture(&image_uniform, &tex, 0);
    tfx_submit(post, out_program, false);

    tfx_frame();

    return 0;
}


void ren_byebye() {
    printf("byebye says the renderer.\n");

    tfx_shutdown();
}