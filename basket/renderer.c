#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>

#include "lib/vec.h"
#include "lib/tinyfx.h"

#define BASKET_INTERNAL
#include "basket.h"
#include "shaders.h"

static int width, height;

typedef vec_t(Vertex) VertexVec;
typedef vec_t(RenderCall) CallVec;
typedef vec_t(Light) LightVec;

static vec_char_t logs;
static CallVec calls;
static CallVec flat_calls;
static VertexVec transient;
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
static bool was_debug = false;
static bool resize; 

static tfx_vertex_format vertex_format;
static f32 view_matrix[16] = IDENTITY_MATRIX;
static f32 proj_matrix[16] = IDENTITY_MATRIX;

static u16 target_w, target_h;
static bool enable_fill;
static f32 scale = 1.0;


static MeshSlice quad = {
    .length = 6
};

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

void ren_videomode(u16 w, u16 h, bool force_ratio) {
    enable_fill = !force_ratio;
    target_w = w;
    target_h = h;
    resize = true;

    SDL_SetWindowMinimumSize(window, w, h);
}

#define TEXTURE_AMOUNT 128
static tfx_texture textures[TEXTURE_AMOUNT];
static tfx_texture texture_none;
static tfx_texture texture_main;
static tfx_texture texture_lumos;

u8 ren_tex_load(const char *data, u32 length) {
    Image tex;
    if (img_init(&tex, data, length))
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

    free(final_source);

    return program;
}


int ren_init(SDL_Window *_window) {
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

    ren_videomode(400, 300, false);

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
    vec_init(&transient);
    vec_init(&flat_calls);
    vec_init(&lights);

    quad.data = malloc(sizeof(Vertex)*6);

    quad.data[0] = (Vertex) { .position = {0.0, 0.0, 0.0}, .uv = {0.0, 0.0}, .color = {.full=0xFFFFFFFF} };
    quad.data[1] = (Vertex) { .position = {1.0, 0.0, 0.0}, .uv = {1.0, 0.0}, .color = {.full=0xFFFFFFFF} };
    quad.data[2] = (Vertex) { .position = {0.0, 1.0, 0.0}, .uv = {0.0, 1.0}, .color = {.full=0xFFFFFFFF} };

    quad.data[3] = (Vertex) { .position = {1.0, 0.0, 0.0}, .uv = {1.0, 0.0}, .color = {.full=0xFFFFFFFF} };
    quad.data[4] = (Vertex) { .position = {0.0, 1.0, 0.0}, .uv = {0.0, 1.0}, .color = {.full=0xFFFFFFFF} };
    quad.data[5] = (Vertex) { .position = {1.0, 1.0, 0.0}, .uv = {1.0, 1.0}, .color = {.full=0xFFFFFFFF} };


    return 0;
}

f32 camera_target[3] = { 0.0f, 0.0f, 0.0f };
void ren_camera(f32 from[3], f32 to[3], f32 up[3]) {
    mat4_lookat(view_matrix, from, to, up);
    mat4_mulvec(camera_target, to, view_matrix);
}

RenderCall *ren_render(RenderCall call) {
    vec_push(&calls, call);
    return &calls.data[calls.length];
}


RenderCall *ren_draw(RenderCall call) {
    vec_push(&flat_calls, call);
    return &flat_calls.data[flat_calls.length];
}

void ren_quad(Quad q) {
    const f32 tmp0[16] = QUICK_SCALE_MATRIX(
        q.texture.w * q.scale[0], 
        q.texture.h * q.scale[1], 
        1.0
    );

    const f32 tmp1[16] = QUICK_TRANSLATION_MATRIX(q.position[0], q.position[1], 0.0);

    RenderCall call = {
        .tint = q.color,
        .texture = q.texture,
        .mesh = quad,
    };
    mat4_mul(call.model, tmp0, tmp1);

    ren_draw(call);
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
        if (w != NULL)
            *w = (u16)ceilf((f32)width /scale);

        if (h != NULL)
            *h = (u16)ceilf((f32)height/scale);
        return;
    }

    if (w != NULL)
        *w = (u16)ceilf((f32)target_w/scale);

    if (h != NULL)
        *h = (u16)ceilf((f32)target_h/scale);
}

void ren_mouse_position(i16 *x, i16 *y) {
    u16 _x, _y;
    eng_mouse_position(&_x, &_y);

    u16 w, h;
    eng_window_size(&w, &h);

    if (x != NULL)
        *x = (_x - ((f32)(w)/2)) / scale;

    if (y != NULL)
        *y = (_y - ((f32)(h)/2)) / scale;

    // TODO: ADD NON_FILL MODE
}

int ren_frame() {
    static tfx_canvas canvas;
    static Frustum frustum;

    static VertexVec tmp_vertices;

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

        if (eng_is_debug())
            flags = 0
                | TFX_RESET_DEBUG_OVERLAY 
                | TFX_RESET_DEBUG_OVERLAY_STATS
                | TFX_RESET_REPORT_GPU_TIMINGS;

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

        if (!tmp_vertices.data)
            vec_init(&tmp_vertices);
    }

    #define CALLCHECK() {                         \
        if (call.disable) continue;               \
        if (call.tint.a == 0) continue;           \
                                                  \
        if (call.texture.w == 0)                  \
            call.texture.w = texture_main.width;  \
                                                  \
        if (call.texture.h == 0)                  \
            call.texture.h = texture_main.height; \
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

        if (frustum_vs_sphere(frustum, position, area * 9)) {
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

    vec_clear(&tmp_vertices);

    for (u32 i = 0; i < calls.length; i++) {
        RenderCall call = calls.data[i];

        CALLCHECK()

        mat4_mul(m, call.model, view_matrix);

        if (!call.range.length)
            call.range = (Range) {
                .offset = 0, 
                .length = call.mesh.length/3
            };

        for (u32 a = call.range.offset; a < call.range.offset+call.range.length; a++) {
            Triangle tri;

            for (u32 b = 0; b < 3; b++) {
                Vertex vertex = call.mesh.data[(a*3)+b];
                Vertex *copy = &tri.arr[b];

                #define _CCM(a,b) (u8) (((unsigned)a * (unsigned)b + 255u) >> 8)
                #define _CKW(a) ((f32)(a) / (f32)texture_main.width)
                #define _CKH(a) ((f32)(a) / (f32)texture_main.height)

                copy->uv[0] = _CKW(call.texture.w) * vertex.uv[0] + _CKW(call.texture.x);
                copy->uv[1] = _CKH(call.texture.h) * vertex.uv[1] + _CKH(call.texture.y);

                u8 fr = _CCM(call.tint.r, vertex.color.r);
                u8 fg = _CCM(call.tint.g, vertex.color.g);
                u8 fb = _CCM(call.tint.b, vertex.color.b);
                u8 fa = _CCM(call.tint.a, vertex.color.a);

                copy->color = (Color) { fr, fg, fb, fa };

                mat4_mulvec(copy->position, vertex.position, m);
            }

            if (frustum_vs_triangle(frustum, tri.a.position, tri.b.position, tri.c.position))
                vec_pusharr(&tmp_vertices, tri.arr, 3);
        }
    }
    vec_clear(&calls);

    Triangle *t_list = (Triangle *)tmp_vertices.data;
    u32 t_amount = tmp_vertices.length/3;

    tfx_transient_buffer buffer = tfx_transient_buffer_new(&vertex_format, t_amount*3);
    memcpy(buffer.data, tmp_vertices.data, t_amount*sizeof(Triangle));

    ren_log("\n// RENDERER //////");
    ren_log("TRIANGLES:  %i", t_amount);
    ren_log("RESOLUTION: %ix%i", width, height);
    ren_log("LIGHTS:     %i", real_index);

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
    tfx_view_set_depth_test(ui, TFX_DEPTH_TEST_LT);
    tfx_view_set_canvas(ui, &canvas, 0);
    tfx_set_texture(&image_uniform, &texture_main, 0);

    vec_clear(&tmp_vertices);

    const u16 w = resolution[0] / 2.f;
    const u16 h = resolution[1] / 2.f;

    for (u32 i = 0; i < flat_calls.length; i++) {
        RenderCall call = flat_calls.data[i];

        CALLCHECK()

        if (!call.range.length)
            call.range = (Range) {
                .offset = 0, 
                .length = call.mesh.length/3
            };

        for (u32 t = call.range.offset; t < call.range.offset+call.range.length; t++) {
            for (u32 j = 0; j < 3; j++) {
                Vertex vertex = call.mesh.data[(t*3)+j];

                #define _CCM(a,b) (u8) (((unsigned)a * (unsigned)b + 255u) >> 8)
                #define _CKW(a) ((f32)(a) / (f32)texture_main.width)
                #define _CKH(a) ((f32)(a) / (f32)texture_main.height)

                Vertex copy = {
                    .uv = {
                        _CKW(call.texture.w) * vertex.uv[0] + _CKW(call.texture.x),
                        _CKH(call.texture.h) * vertex.uv[1] + _CKH(call.texture.y)
                    },
                };

                u8 fr = _CCM(call.tint.r, vertex.color.r);
                u8 fg = _CCM(call.tint.g, vertex.color.g);
                u8 fb = _CCM(call.tint.b, vertex.color.b);
                u8 fa = _CCM(call.tint.a, vertex.color.a);

                copy.color = (Color) { fr, fg, fb, fa };

                mat4_mulvec(copy.position, vertex.position, call.model);

                vec_push(&tmp_vertices, copy);
            }
        }
    }

    if (tmp_vertices.length % 3 != 0)
        printf("?\n");

    tfx_transient_buffer quad_buffer = tfx_transient_buffer_new(&vertex_format, tmp_vertices.length);
    memcpy(quad_buffer.data, tmp_vertices.data, tmp_vertices.length * sizeof(Vertex));
    t_amount += tmp_vertices.length;

    tfx_set_transient_buffer(quad_buffer);
    tfx_submit(ui, quad_program, false);

    const f32 size = (float)(t_amount * sizeof(Triangle)) / 1024.0f;
    ren_log("GPU UPLOADS: (%.3gkb)", size);

    vec_clear(&flat_calls);

    const f32 transient_mem = (float)(transient.length * sizeof(Triangle)) / 1024.0f;
    ren_log("TRANSIENT:   (%.3gkb)", transient_mem);

    vec_clear(&transient);


    // RENDER OUTPUT
    const u8 post = 4;
    tfx_set_state(TFX_STATE_RGB_WRITE);
    tfx_view_set_depth_test(post, TFX_DEPTH_TEST_NONE);
	tfx_view_set_name(post, "the output pass");

    tfx_transient_buffer flat_quad = tfx_transient_buffer_new(&vertex_format, 6);
    Vertex quad[6] = {
    //    {{ -1.0, -1.0, 0.0 }},
    //    {{ -1.0,  1.0, 0.0 }},
    //    {{  1.0, -1.0, 0.0 }},
    //    {{  1.0,  1.0, 0.0 }},
    //    {{  1.0, -1.0, 0.0 }},
    //    {{ -1.0,  1.0, 0.0 }},
    };
    u8 index = 0;

    #define PUSH(x, y, u, v) \
        quad[index++] = (Vertex) { { x/(f32)(width), y/(f32)(height), 0.0 }, { u, v }, { .full=0xFFFFFFFF } }

    PUSH(-(resolution[0])*scale, -(resolution[1])*scale, 0, 0);
    PUSH(-(resolution[0])*scale,  (resolution[1])*scale, 0, 1);
    PUSH( (resolution[0])*scale, -(resolution[1])*scale, 1, 0);

    PUSH( (resolution[0])*scale,  (resolution[1])*scale, 1, 1);
    PUSH( (resolution[0])*scale, -(resolution[1])*scale, 1, 0);
    PUSH(-(resolution[0])*scale,  (resolution[1])*scale, 0, 1);

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

    vec_deinit(&logs);
    vec_deinit(&calls);
    vec_deinit(&transient);
    vec_deinit(&flat_calls);
    vec_deinit(&lights);

    free(quad.data);

    tfx_shutdown();
}