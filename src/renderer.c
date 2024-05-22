// implement renderer, the single most advanced piece of code in src/
// do not be afraid, embrace it.

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vec.h"
#include "tinyfx.h"
#include "cute_png.h"
#include "common.h"
#include "font.h"

#define REN_ATLAS_WIDTH 512
#define REN_ATLAS_HEIGHT 512
#define REN_PIXEL_SIZE 1 // Might turn into a variable?

static int width, height;
static bool resized = false;

typedef vec_t(Vertex) VertexVec;
typedef vec_t(Quad) QuadVec;
typedef vec_t(RenderCall) CallVec;

static CallVec calls;
static QuadVec quads; 
static Light lights[32];
static int light_index = 0;

static tfx_uniform proj_uniform;
static tfx_uniform image_uniform;
static tfx_uniform lumos_uniform;
static tfx_uniform res_uniform;
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

static f32 far = 15.0;
static Color clear_color = { .full = 0x12002EFF };
static Color ambient = { .full=0x9966CCFF };
static int snapping = 0;
static bool dither = true;

static vec_char_t logs;


static tfx_vertex_format vertex_format;
static f32 view_matrix[16] = IDENTITY_MATRIX;

static char *shader_source = NULL;
static char *flat_shader_source = NULL;
static char *wire_shader_source = NULL;
static char *quad_shader_source = NULL;

static tfx_buffer flat_quad;

SDL_Window *window;

static void tfx_debug_thingy(const char *str, tfx_severity severity) {
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

bool ren_init(SDL_Window *_window) {
    printf("setting up renderer.\n");

    window = _window;

    // setup opengl bullshit
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    i8 vsync = 1;
    if (SDL_GL_ExtensionSupported("EXT_swap_control_tear"))
        vsync = -1;

    if (getenv("BK_NO_VSYNC"))
        vsync = 0;

    SDL_GL_SetSwapInterval(vsync);

	SDL_GLContext *context = SDL_GL_CreateContext(window);
	SDL_GL_GetDrawableSize(window, (int*)&width, (int*)&height);
	SDL_GL_MakeCurrent(window, context);

    SDL_SetWindowMinimumSize(window, 600, 600);

    // setup tinyfx stuff
    tfx_platform_data pd;
	pd.use_gles = true;
	pd.context_version = 30;
	pd.gl_get_proc_address = SDL_GL_GetProcAddress;
#ifdef TRASH_DEBUG
    pd.info_log = tfx_debug_thingy;
#endif

	tfx_set_platform_data(pd);

	vertex_format = tfx_vertex_format_start();
	tfx_vertex_format_add(&vertex_format, 0, 3, false, TFX_TYPE_FLOAT); // Position
	tfx_vertex_format_add(&vertex_format, 1, 2, false, TFX_TYPE_FLOAT); // UV
	tfx_vertex_format_add(&vertex_format, 2, 4, false, TFX_TYPE_UBYTE); // Color
	tfx_vertex_format_end(&vertex_format);

    proj_uniform     = tfx_uniform_new("projection", TFX_UNIFORM_MAT4,  1);
    image_uniform    = tfx_uniform_new("image",      TFX_UNIFORM_INT,   1);
    lumos_uniform    = tfx_uniform_new("lumos",      TFX_UNIFORM_INT,   1);
    res_uniform      = tfx_uniform_new("resolution", TFX_UNIFORM_VEC2,  1);
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

    resized = true;

    shader_source      = (char *)fs_read("assets/shd_shader.glsl",    0);
    flat_shader_source = (char *)fs_read("assets/shd_output.glsl",    0);
    quad_shader_source = (char *)fs_read("assets/shd_2D.glsl",    0);

    vec_init(&logs);
    vec_init(&calls);
    vec_init(&quads);

    return false;
}

void ren_resize(u16 width_, u16 height_) {
    width = width_;
    height = height_;

    resized = true;
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
    lights[light_index++] = light;
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
    static char buffer[128];

    va_list argptr;
    va_start(argptr, str);
    vsnprintf(buffer, 128, str, argptr);
    va_end(argptr);

    u32 len = strlen(buffer);
    buffer[len] = '\n';

    vec_pusharr(&logs, buffer, len+1);
}

void ren_far(f32 f, Color clear) {
    clear_color = clear;
    far = f;
}

void ren_ambient(Color amb) {
    ambient = amb;
}

void ren_snapping(u8 snap) {
    snapping = snap;
}

void ren_dithering(bool dither_) {
    dither = dither_;
}

int ren_frame() {
    static tfx_canvas canvas;
    static tfx_program program;
    static tfx_program out_program;
    static tfx_program quad_program;
    static tfx_texture atlas;
    static tfx_texture lumos;
    static tfx_texture font_texture;
    static Frustum frustum;

    static VertexVec scene_triangles;

    static f32 scale = 1.0;

    static f32 proj_matrix[16];

    int curr_width, curr_height;
    SDL_GL_GetDrawableSize(window, &curr_width, &curr_height);

    if (curr_width != width || curr_height != height)
        ren_resize(curr_width, curr_height);

    if (resized) {
        resized = false;

        tfx_reset_flags flags = TFX_RESET_NONE;

#ifdef TRASH_DEBUG
        flags = 0
            | TFX_RESET_DEBUG_OVERLAY 
            | TFX_RESET_DEBUG_OVERLAY_STATS
            | TFX_RESET_REPORT_GPU_TIMINGS;
#endif

        tfx_reset(width, height, flags);

        if (canvas.gl_fbo[0] > 0)
            tfx_canvas_free(&canvas);

        canvas = tfx_canvas_new(
            width/REN_PIXEL_SIZE, 
            height/REN_PIXEL_SIZE, 
            TFX_FORMAT_RGBA8_D24, 
            TFX_TEXTURE_FILTER_POINT
        );

        if (!program) {
            const char *attribs[] = {
                "vx_position",
                "vx_uv",
                "vx_color",
                NULL
            };

            program = tfx_program_new (
                (const char*)shader_source, 
                (const char*)shader_source, 
                attribs, -1
            );

            out_program = tfx_program_new (
                (const char*)flat_shader_source, 
                (const char*)flat_shader_source, 
                attribs, -1
            );

            quad_program = tfx_program_new (
                (const char*)quad_shader_source, 
                (const char*)quad_shader_source, 
                attribs, -1
            );
        }

        if (!atlas.gl_count) {
            u32 length = 0; 
            {
                const char *raw = fs_read("assets/tex_atlas.png", &length);
                cp_image_t img = cp_load_png_mem(raw, length);
                atlas = tfx_texture_new(img.w, img.h, 1, img.pix, TFX_FORMAT_RGBA8, TFX_TEXTURE_FILTER_POINT);
                cp_free_png(&img);
            }

            {
                const char *raw = fs_read("assets/tex_lumos.png", &length);
                cp_image_t img = cp_load_png_mem(raw, length);
                lumos = tfx_texture_new(img.w, img.h, 1, img.pix, TFX_FORMAT_RGBA8, TFX_TEXTURE_FILTER_POINT);
                cp_free_png(&img);
            }
        }

        if (!flat_quad.gl_id) {
            Vertex quad[6] = {
                {{ -1.0, -1.0, 0.0 }},
                {{ -1.0,  1.0, 0.0 }},
                {{  1.0, -1.0, 0.0 }},

                {{  1.0,  1.0, 0.0 }},
                {{  1.0, -1.0, 0.0 }},
                {{ -1.0,  1.0, 0.0 }},
            };

            flat_quad = tfx_buffer_new(quad, sizeof(quad), &vertex_format, TFX_BUFFER_NONE);
        }

        f32 s = (f32)REN_PIXEL_SIZE;
        f32 res[2] = { (f32)(width)/s, (f32)(height)/s };
        tfx_set_uniform(&res_uniform, res, 1);

        int pixelsize = REN_PIXEL_SIZE;
        tfx_set_uniform_int(&scale_uniform, &pixelsize, 1);
        
        scale = max(1.0, floorf(min(res[0], res[1])/300.f));

        if (!scene_triangles.data)
            vec_init(&scene_triangles);
    }

    f32 aspect = (f32)width / (f32)height;

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
    f32 light_positions[16*3];
    f32 light_colors[16*3];

    for (u8 i = 0; i < light_index; i++) {
        Light light = lights[i];

        f32 area = vec_len(light.color, 3);

        f32 *where = &light_positions[real_index * 3];

        mat4_mulvec(where, light.position, view_matrix);

        if (frustum_vs_sphere(frustum, where, area * 2)) {
            memcpy(&light_colors[real_index * 3], light.color, sizeof(light.color));

            if (real_index++ == 16)
                break;
        }
    }
    light_index = 0;

    if (real_index) {
        tfx_set_uniform(&lposition_uniform, light_positions, -1);
        tfx_set_uniform(&lcolor_uniform, light_colors, -1);
    }

    tfx_set_uniform_int(&lamount_uniform, &real_index, 1);

    tfx_set_uniform_int(&dither_uniform, (int *)&dither, 1);

    tfx_set_state(TFX_STATE_RGB_WRITE | TFX_STATE_DEPTH_WRITE);

    // RENDER TRIANGLES
    u8 view = 1;
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
            call.texture.w = atlas.width;
            call.texture.h = atlas.height;
        }

        mat4_mul(m, call.model, view_matrix);

        //printf("sexo? %i\n", call.mesh.length);

        for (u32 a = 0; a < call.mesh.length; a+=3) {
            Triangle tri;

            for (u32 b = 0; b < 3; b++) {
                Vertex vertex = call.mesh.data[a+b];
                Vertex *copy = &tri.arr[b];

                #define _CCM(a,b) (u8) (((unsigned)a * (unsigned)b + 255u) >> 8)
                #define _CKW(a) ((f32)(a) / (f32)atlas.width)
                #define _CKH(a) ((f32)(a) / (f32)atlas.height)
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
    u32 t_amount = scene_triangles.length/3;

    tfx_transient_buffer buffer = tfx_transient_buffer_new(&vertex_format, t_amount*3);
    memcpy(buffer.data, scene_triangles.data, t_amount*sizeof(Triangle));

    vec_clear(&scene_triangles);

    static char tmp_str[128];

    ren_log("\n// RENDERER //////");
    ren_log("TRIANGLES:  %i", t_amount);
    ren_log("RESOLUTION: %ix%i", width, height);
    ren_log("QUADS:      %i", quads.length);
    ren_log("LIGHTS:     %i", real_index);

    u32 s = (quads.length * 2 * sizeof(Triangle)) +
            (t_amount * sizeof(Triangle));

    f32 size = (float)(s) / 1024.0f;
    ren_log("GPU UPLOADS: (%.3gkb)", size);

    vec_push(&logs, 0);

    tfx_debug_print(8, 0, 9, 5, logs.data);

    vec_clear(&logs);

    tfx_set_transient_buffer(buffer);
    tfx_set_texture(&image_uniform, &atlas, 0);
    tfx_set_texture(&lumos_uniform, &lumos, 1);
    tfx_submit(view, program, false);
    

    // RENDER QUADS
    u8 ui = 3;
    tfx_set_state(TFX_STATE_RGB_WRITE);
    tfx_view_set_name(ui, "the quad pass");
    tfx_view_set_canvas(ui, &canvas, 0);
    tfx_set_texture(&image_uniform, &atlas, 0);
    tfx_transient_buffer quad_buffer = tfx_transient_buffer_new(&vertex_format, quads.length*6);

    f32 w = (f32)(width  / scale) / 2.f;
    f32 h = (f32)(height / scale) / 2.f;

    mat4_ortho(m, -w, w, -h, h, 0.01f, 100.f);

    Vertex *quad_vertices = quad_buffer.data;

    for (u32 i = 0; i < quads.length; i++) {
        Quad q = quads.data[i];

        f32 tsx = (q.texture.x) / (f32)atlas.width;
        f32 tsy = (q.texture.y) / (f32)atlas.height;
        f32 tcx = (q.texture.x + q.texture.w) / (f32)atlas.width;
        f32 tcy = (q.texture.y + q.texture.h) / (f32)atlas.height;

        f32 qsx = q.position[0];
        f32 qsy = q.position[1];
        f32 qcx = q.position[0] + (q.texture.w * q.scale[0]);
        f32 qcy = q.position[1] + (q.texture.h * q.scale[1]);

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
    u8 post = 4;
    tfx_set_state(TFX_STATE_RGB_WRITE);
    tfx_view_set_depth_test(post, TFX_DEPTH_TEST_NONE);
	tfx_view_set_name(post, "the output pass");
    tfx_texture tex = tfx_get_texture(&canvas, 0);
    tfx_set_texture(&image_uniform, &tex, 0);
    tfx_set_vertices(&flat_quad, 6);
    tfx_submit(post, out_program, false);

    tfx_frame();

    return 0;
}


void ren_byebye() {
    printf("byebye says the renderer.\n");

    tfx_shutdown();
}