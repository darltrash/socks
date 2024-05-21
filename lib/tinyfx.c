#define TFX_IMPLEMENTATION
#if !defined(TFX_DEBUG) && defined(_DEBUG)
// enable tfx debug in msvc debug configurations.
#define TFX_DEBUG
#endif
#include "tinyfx.h"

#ifdef TFX_LEAK_CHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#endif

/**********************\
| implementation stuff |
\**********************/
#ifdef TFX_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

// TODO: look into just keeping the stuff from GL header in here, this thing
// isn't included on many systems and is kind of annoying to always need.
// neil: I straight up changed this from a system-level file to project-level
// because it's a pain in the ass.
#include "GL/glcorearb.h"
//#ifdef __ANDROID__
//#include <GLES3/gl32.h>
//#endif
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#ifdef TFX_DEBUG
#include <assert.h>
#else
#define assert(op) (void)(op);
#endif

// needed on msvc
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif

#ifndef TFX_TRANSIENT_BUFFER_COUNT
#define TFX_TRANSIENT_BUFFER_COUNT 3
#endif

#ifndef TFX_UNIFORM_BUFFER_SIZE
// by default, allow up to 4MB of uniform updates per frame.
#define TFX_UNIFORM_BUFFER_SIZE 1024*1024*4
#endif

#ifndef TFX_TRANSIENT_BUFFER_SIZE
// by default, allow up to 4MB of transient buffer data per frame.
#define TFX_TRANSIENT_BUFFER_SIZE 1024*1024*4
#endif

// The following code is public domain, from https://github.com/nothings/stb
//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
#define STB_STRETCHY_BUFFER_CPP
#endif
#ifndef STB_STRETCHY_BUFFER_H_INCLUDED
#define STB_STRETCHY_BUFFER_H_INCLUDED

#ifndef NO_STRETCHY_BUFFER_SHORT_NAMES
#define sb_free   stb_sb_free
#define sb_push   stb_sb_push
#define sb_count  stb_sb_count
#define sb_add    stb_sb_add
#define sb_last   stb_sb_last
#endif

#ifdef  STB_STRETCHY_BUFFER_CPP
#define stb_sb_push(t,a,v)      (stb__sbmaybegrow(t,a,1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_add(t,a,n)       (stb__sbmaybegrow(t,a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb__sbmaybegrow(t,a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(t,a,n) : 0)
#define stb__sbgrow(t,a,n)      ((a) = (t*)stb__sbgrowf((void*)(a), (n), sizeof(t)))
#else
#define stb_sb_push(a,v)        (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_add(a,n)         (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb__sbmaybegrow(a,n)   (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)        ((a) = stb__sbgrowf((a), (n), sizeof(*(a))))
#endif

#define stb_sb_free(a)          ((a) ? free(stb__sbraw(a)),0 : 0)
#define stb_sb_count(a)         ((a) ? stb__sbn(a) : 0)
#define stb_sb_last(a)          ((a)[stb__sbn(a)-1])

#define stb__sbraw(a) ((int *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))

#include <stdlib.h>

static void * stb__sbgrowf(void *arr, int increment, int itemsize)
{
	int dbl_cur = arr ? 2 * stb__sbm(arr) : 0;
	int min_needed = stb_sb_count(arr) + increment;
	int m = dbl_cur > min_needed ? dbl_cur : min_needed;
	int *p = (int *)realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int) * 2);
	if (p) {
		if (!arr)
			p[1] = 0;
		p[0] = m;
		return p + 2;
	}
	else {
#ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
		STRETCHY_BUFFER_OUT_OF_MEMORY;
#endif
		return (void *)(2 * sizeof(int)); // try to force a NULL pointer exception later
	}
}
#endif // STB_STRETCHY_BUFFER_H_INCLUDED
//////////////////////////////////////////////////////////////////////////////

static char *tfx_strdup(const char *src) {
	size_t len = strlen(src) + 1;
	char *s = malloc(len);
	if (s == NULL)
		return NULL;
	// copy len includes the \0 terminator
	return (char *)memcpy(s, src, len);
}

#ifdef TFX_DEBUG
//#define TFX_FATAL_ERRORS
#	ifdef TFX_FATAL_ERRORS
#		define CHECK(fn) fn; { GLenum _status; while ((_status = tfx_glGetError())) { if (_status == GL_NO_ERROR) break; TFX_ERROR("%s:%d GL ERROR: %d", __FILE__, __LINE__, _status); assert(0); } }
#	else
#		define CHECK(fn) fn; { GLenum _status; while ((_status = tfx_glGetError())) { if (_status == GL_NO_ERROR) break; TFX_ERROR("%s:%d GL ERROR: %d", __FILE__, __LINE__, _status); } }
#	endif
#else
#	define CHECK(fn) fn;
#endif

#define TFX_INFO(msg, ...) tfx_printf(TFX_SEVERITY_INFO, msg, __VA_ARGS__)
#define TFX_WARN(msg, ...) tfx_printf(TFX_SEVERITY_WARNING, msg, __VA_ARGS__)
#define TFX_ERROR(msg, ...) tfx_printf(TFX_SEVERITY_ERROR, msg, __VA_ARGS__)
#define TFX_FATAL(msg, ...) tfx_printf(TFX_SEVERITY_FATAL, msg, __VA_ARGS__)

#define VIEW_MAX 256
#define TIMER_LATENCY 3
#define TIMER_COUNT ((VIEW_MAX+1)*TIMER_LATENCY)

// view flags
enum {
	// clear modes
	TFXI_VIEW_CLEAR_COLOR     = 1 << 0,
	TFXI_VIEW_CLEAR_DEPTH     = 1 << 1,

	// depth modes
	TFXI_VIEW_DEPTH_TEST_LT   = 1 << 2,
	TFXI_VIEW_DEPTH_TEST_GT   = 1 << 3,
	TFXI_VIEW_DEPTH_TEST_EQ   = 1 << 4,

	// scissor test
	TFXI_VIEW_SCISSOR         = 1 << 5,

	TFXI_VIEW_INVALIDATE      = 1 << 6,
	TFXI_VIEW_FLUSH           = 1 << 7,
	TFXI_VIEW_SORT_SEQUENTIAL = 1 << 8
};

typedef struct tfx_rect {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
} tfx_rect;

typedef struct tfx_blit_op {
	tfx_canvas *source;
	int source_mip;
	tfx_rect rect;
	GLenum mask;
} tfx_blit_op;

typedef struct tfx_draw {
	tfx_draw_callback callback;
	uint64_t flags;

	tfx_program program;
	tfx_uniform *uniforms;

	tfx_texture textures[8];
	uint8_t textures_mip[8];
	bool textures_write[8];
	tfx_buffer ssbos[8];
	bool ssbo_write[8];
	tfx_buffer vbo;
	bool use_vbo;

	tfx_buffer ibo;
	bool use_ibo;

	tfx_vertex_format tvb_fmt;
	bool use_tvb;

	tfx_rect scissor_rect;
	bool use_scissor;

	size_t offset;
	uint32_t indices;
	uint32_t depth;

	// for compute jobs
	uint32_t threads_x;
	uint32_t threads_y;
	uint32_t threads_z;
} tfx_draw;

typedef struct tfx_view {
	uint32_t flags;

	const char *name;

	bool has_canvas;
	tfx_canvas  canvas;
	int canvas_layer;

	tfx_draw    *draws;
	tfx_draw    *jobs;
	tfx_blit_op *blits;

	unsigned clear_color;
	float clear_depth;

	tfx_rect scissor_rect;
	// https://opengl.gpuinfo.org/displaycapability.php?name=GL_MAX_VIEWPORTS
	tfx_rect viewports[16];
	int viewport_count;

	// for single pass rendering to eyes, cubes, shadowmaps etc.
	int instance_mul;

	float view[16];
	float proj_left[16];
	float proj_right[16];
} tfx_view;

#define TFXI_VIEW_CLEAR_MASK      (TFXI_VIEW_CLEAR_COLOR | TFXI_VIEW_CLEAR_DEPTH)
#define TFXI_VIEW_DEPTH_TEST_MASK (TFXI_VIEW_DEPTH_TEST_LT | TFXI_VIEW_DEPTH_TEST_GT | TFXI_VIEW_DEPTH_TEST_EQ)

#define TFXI_STATE_CULL_MASK      (TFX_STATE_CULL_CW | TFX_STATE_CULL_CCW)
#define TFXI_STATE_BLEND_MASK     (TFX_STATE_BLEND_ALPHA)
#define TFXI_STATE_DRAW_MASK      (TFX_STATE_DRAW_POINTS \
	| TFX_STATE_DRAW_LINES | TFX_STATE_DRAW_LINE_STRIP | TFX_STATE_DRAW_LINE_LOOP \
	| TFX_STATE_DRAW_TRI_STRIP | TFX_STATE_DRAW_TRI_FAN \
)

static tfx_canvas g_backbuffer;
static tfx_timing_info last_timings[VIEW_MAX];
static tfx_platform_data g_platform_data;

typedef struct tfx_glext {
	const char *ext;
	bool supported;
} tfx_glext;

static tfx_glext available_exts[] = {
	{ "GL_ARB_multisample", false },
	{ "GL_ARB_compute_shader", false },
	{ "GL_ARB_texture_float", false },
	{ "GL_EXT_debug_marker", false },
	{ "GL_ARB_debug_output", false },
	{ "GL_KHR_debug", false },
	{ "GL_NVX_gpu_memory_info", false },
	// guaranteed by desktop GL 3.3+ or GLES 3.0+
	{ "GL_ARB_instanced_arrays", false },
	{ "GL_ARB_seamless_cube_map", false },
	{ "GL_EXT_texture_filter_anisotropic", false },
	{ "GL_ARB_multi_bind", false },
	// TODO
	// GL_AMD_vertex_shader_layer
	// GL_AMD_vertex_shader_viewport_index
	// GL_ARB_multi_bind
	// GL_ARB_multi_draw_indirect
	// GL_QCOM_texture_foveated
	// GL_OVR_multiview2
	// GL_OVR_multiview_multisampled_render_to_texture
	// GL_OES_element_index_uint
	// GL_OES_depth24
	// GL_OES_depth_texture
	// GL_OES_depth_texture_cube_map
	// GL_EXT_sRGB
	// GL_EXT_multisampled_render_to_texture
	// GL_EXT_disjoint_timer_query
	// GL_EXT_clip_control
	// GL_EXT_clip_cull_distance
	// GL_EXT_color_buffer_float
	// GL_EXT_color_buffer_half_float
	{ NULL, false }
};

PFNGLFLUSHPROC tfx_glFlush;
PFNGLGETSTRINGPROC tfx_glGetString;
PFNGLGETSTRINGIPROC tfx_glGetStringi;
PFNGLGETERRORPROC tfx_glGetError;
PFNGLBLENDFUNCPROC tfx_glBlendFunc;
PFNGLCOLORMASKPROC tfx_glColorMask;
PFNGLGETINTEGERVPROC tfx_glGetIntegerv;
PFNGLGETFLOATVPROC tfx_glGetFloatv;
PFNGLGENQUERIESPROC tfx_glGenQueries;
PFNGLDELETEQUERIESPROC tfx_glDeleteQueries;
PFNGLBEGINQUERYPROC tfx_glBeginQuery;
PFNGLENDQUERYPROC tfx_glEndQuery;
PFNGLQUERYCOUNTERPROC tfx_glQueryCounter;
PFNGLGETQUERYOBJECTUIVPROC tfx_glGetQueryObjectuiv;
PFNGLGETQUERYOBJECTUI64VPROC tfx_glGetQueryObjectui64v;
PFNGLGENBUFFERSPROC tfx_glGenBuffers;
PFNGLBINDBUFFERPROC tfx_glBindBuffer;
PFNGLBUFFERDATAPROC tfx_glBufferData;
PFNGLBUFFERSTORAGEPROC tfx_glBufferStorage;
PFNGLDELETEBUFFERSPROC tfx_glDeleteBuffers;
PFNGLDELETETEXTURESPROC tfx_glDeleteTextures;
PFNGLCREATESHADERPROC tfx_glCreateShader;
PFNGLSHADERSOURCEPROC tfx_glShaderSource;
PFNGLCOMPILESHADERPROC tfx_glCompileShader;
PFNGLGETSHADERIVPROC tfx_glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC tfx_glGetShaderInfoLog;
PFNGLDELETESHADERPROC tfx_glDeleteShader;
PFNGLCREATEPROGRAMPROC tfx_glCreateProgram;
PFNGLATTACHSHADERPROC tfx_glAttachShader;
PFNGLBINDATTRIBLOCATIONPROC tfx_glBindAttribLocation;
PFNGLLINKPROGRAMPROC tfx_glLinkProgram;
PFNGLGETPROGRAMIVPROC tfx_glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC tfx_glGetProgramInfoLog;
PFNGLDELETEPROGRAMPROC tfx_glDeleteProgram;
PFNGLGENTEXTURESPROC tfx_glGenTextures;
PFNGLBINDTEXTUREPROC tfx_glBindTexture;
PFNGLBINDTEXTURESPROC tfx_glBindTextures;
PFNGLTEXPARAMETERIPROC tfx_glTexParameteri;
PFNGLTEXPARAMETERIVPROC tfx_glTexParameteriv;
PFNGLTEXPARAMETERFPROC tfx_glTexParameterf;
PFNGLPIXELSTOREIPROC tfx_glPixelStorei;
PFNGLTEXIMAGE2DPROC tfx_glTexImage2D;
PFNGLTEXIMAGE3DPROC tfx_glTexImage3D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC tfx_glTexImage2DMultisample;
PFNGLTEXSTORAGE2DPROC tfx_glTexStorage2D;
PFNGLTEXSTORAGE3DPROC tfx_glTexStorage3D;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC tfx_glTexStorage2DMultisample;
PFNGLTEXSUBIMAGE2DPROC tfx_glTexSubImage2D;
PFNGLTEXSUBIMAGE3DPROC tfx_glTexSubImage3D;
PFNGLINVALIDATETEXSUBIMAGEPROC tfx_glInvalidateTexSubImage;
PFNGLGENERATEMIPMAPPROC tfx_glGenerateMipmap;
PFNGLGENFRAMEBUFFERSPROC tfx_glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC tfx_glDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC tfx_glBindFramebuffer;
PFNGLBLITFRAMEBUFFERPROC tfx_glBlitFramebuffer;
PFNGLCOPYIMAGESUBDATAPROC tfx_glCopyImageSubData;
PFNGLFRAMEBUFFERTEXTURE2DPROC tfx_glFramebufferTexture2D;
PFNGLINVALIDATEFRAMEBUFFERPROC tfx_glInvalidateFramebuffer;
PFNGLGENRENDERBUFFERSPROC tfx_glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC tfx_glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC tfx_glRenderbufferStorage;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC tfx_glRenderbufferStorageMultisample;
PFNGLFRAMEBUFFERRENDERBUFFERPROC tfx_glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTUREPROC tfx_glFramebufferTexture;
PFNGLDRAWBUFFERSPROC tfx_glDrawBuffers;
PFNGLREADBUFFERPROC tfx_glReadBuffer;
PFNGLCHECKFRAMEBUFFERSTATUSPROC tfx_glCheckFramebufferStatus;
PFNGLGETUNIFORMLOCATIONPROC tfx_glGetUniformLocation;
PFNGLRELEASESHADERCOMPILERPROC tfx_glReleaseShaderCompiler;
PFNGLGENVERTEXARRAYSPROC tfx_glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC tfx_glBindVertexArray;
PFNGLMAPBUFFERRANGEPROC tfx_glMapBufferRange;
PFNGLBUFFERSUBDATAPROC tfx_glBufferSubData;
PFNGLUNMAPBUFFERPROC tfx_glUnmapBuffer;
PFNGLUSEPROGRAMPROC tfx_glUseProgram;
PFNGLMEMORYBARRIERPROC tfx_glMemoryBarrier;
PFNGLBINDBUFFERBASEPROC tfx_glBindBufferBase;
PFNGLDISPATCHCOMPUTEPROC tfx_glDispatchCompute;
PFNGLVIEWPORTPROC tfx_glViewport;
PFNGLVIEWPORTINDEXEDFPROC tfx_glViewportIndexedf;
PFNGLSCISSORPROC tfx_glScissor;
PFNGLCLEARCOLORPROC tfx_glClearColor;
PFNGLCLEARDEPTHFPROC tfx_glClearDepthf;
PFNGLCLEARPROC tfx_glClear;
PFNGLENABLEPROC tfx_glEnable;
PFNGLDEPTHFUNCPROC tfx_glDepthFunc;
PFNGLDISABLEPROC tfx_glDisable;
PFNGLDEPTHMASKPROC tfx_glDepthMask;
PFNGLFRONTFACEPROC tfx_glFrontFace;
PFNGLPOLYGONMODEPROC tfx_glPolygonMode;
PFNGLUNIFORM1IVPROC tfx_glUniform1iv;
PFNGLUNIFORM1FVPROC tfx_glUniform1fv;
PFNGLUNIFORM2FVPROC tfx_glUniform2fv;
PFNGLUNIFORM3FVPROC tfx_glUniform3fv;
PFNGLUNIFORM4FVPROC tfx_glUniform4fv;
PFNGLUNIFORMMATRIX2FVPROC tfx_glUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC tfx_glUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC tfx_glUniformMatrix4fv;
PFNGLENABLEVERTEXATTRIBARRAYPROC tfx_glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC tfx_glVertexAttribPointer;
PFNGLDISABLEVERTEXATTRIBARRAYPROC tfx_glDisableVertexAttribArray;
PFNGLACTIVETEXTUREPROC tfx_glActiveTexture;
PFNGLDRAWELEMENTSINSTANCEDPROC tfx_glDrawElementsInstanced;
PFNGLDRAWARRAYSINSTANCEDPROC tfx_glDrawArraysInstanced;
PFNGLDRAWELEMENTSPROC tfx_glDrawElements;
PFNGLDRAWARRAYSPROC tfx_glDrawArrays;
PFNGLDELETEVERTEXARRAYSPROC tfx_glDeleteVertexArrays;
PFNGLBINDFRAGDATALOCATIONPROC tfx_glBindFragDataLocation;

// debug output/markers
PFNGLPUSHDEBUGGROUPPROC tfx_glPushDebugGroup;
PFNGLPOPDEBUGGROUPPROC tfx_glPopDebugGroup;
PFNGLINSERTEVENTMARKEREXTPROC tfx_glInsertEventMarkerEXT;

void load_em_up(void* (*get_proc_address)(const char*)) {
	tfx_glFlush = get_proc_address("glFlush");
	tfx_glGetString = get_proc_address("glGetString");
	tfx_glGetStringi = get_proc_address("glGetStringi");
	tfx_glGetError = get_proc_address("glGetError");
	tfx_glBlendFunc = get_proc_address("glBlendFunc");
	tfx_glColorMask = get_proc_address("glColorMask");
	tfx_glGetIntegerv = get_proc_address("glGetIntegerv");
	tfx_glGetFloatv = get_proc_address("glGetFloatv");
	tfx_glGenQueries = get_proc_address("glGenQueries");
	tfx_glDeleteQueries = get_proc_address("glDeleteQueries");
	tfx_glBeginQuery = get_proc_address("glBeginQuery");
	tfx_glEndQuery = get_proc_address("glEndQuery");
	tfx_glQueryCounter = get_proc_address("glQueryCounter");
	tfx_glGetQueryObjectuiv = get_proc_address("glGetQueryObjectuiv");
	tfx_glGetQueryObjectui64v = get_proc_address("glGetQueryObjectui64v");
	tfx_glGenBuffers = get_proc_address("glGenBuffers");
	tfx_glBindBuffer = get_proc_address("glBindBuffer");
	tfx_glBufferData = get_proc_address("glBufferData");
	tfx_glBufferStorage = get_proc_address("glBufferStorage");
	tfx_glDeleteBuffers = get_proc_address("glDeleteBuffers");
	tfx_glDeleteTextures= get_proc_address("glDeleteTextures");
	tfx_glCreateShader = get_proc_address("glCreateShader");
	tfx_glShaderSource = get_proc_address("glShaderSource");
	tfx_glCompileShader = get_proc_address("glCompileShader");
	tfx_glGetShaderiv = get_proc_address("glGetShaderiv");
	tfx_glGetShaderInfoLog = get_proc_address("glGetShaderInfoLog");
	tfx_glDeleteShader = get_proc_address("glDeleteShader");
	tfx_glCreateProgram = get_proc_address("glCreateProgram");
	tfx_glAttachShader = get_proc_address("glAttachShader");
	tfx_glBindAttribLocation = get_proc_address("glBindAttribLocation");
	tfx_glLinkProgram = get_proc_address("glLinkProgram");
	tfx_glGetProgramiv = get_proc_address("glGetProgramiv");
	tfx_glGetProgramInfoLog = get_proc_address("glGetProgramInfoLog");
	tfx_glDeleteProgram = get_proc_address("glDeleteProgram");
	tfx_glGenTextures = get_proc_address("glGenTextures");
	tfx_glBindTexture = get_proc_address("glBindTexture");
	tfx_glBindTextures = get_proc_address("glBindTextures"); // GL_ARB_multi_bind (GL 4.4)
	tfx_glTexParameteri = get_proc_address("glTexParameteri");
	tfx_glTexParameteriv = get_proc_address("glTexParameteriv");
	tfx_glTexParameterf = get_proc_address("glTexParameterf");
	tfx_glPixelStorei = get_proc_address("glPixelStorei");
	tfx_glTexImage2D = get_proc_address("glTexImage2D");
	tfx_glTexImage3D = get_proc_address("glTexImage3D");
	tfx_glTexImage2DMultisample = get_proc_address("glTexImage2DMultisample");
	tfx_glTexStorage2D = get_proc_address("glTexStorage2D");
	tfx_glTexStorage3D = get_proc_address("glTexStorage3D");
	tfx_glTexStorage2DMultisample = get_proc_address("glTexStorage2DMultisample");
	tfx_glTexSubImage2D = get_proc_address("glTexSubImage2D");
	tfx_glTexSubImage3D = get_proc_address("glTexSubImage3D");
	tfx_glInvalidateTexSubImage = get_proc_address("glInvalidateTexSubImage");
	tfx_glGenerateMipmap = get_proc_address("glGenerateMipmap");
	tfx_glGenFramebuffers = get_proc_address("glGenFramebuffers");
	tfx_glDeleteFramebuffers = get_proc_address("glDeleteFramebuffers");
	tfx_glBindFramebuffer = get_proc_address("glBindFramebuffer");
	tfx_glBlitFramebuffer = get_proc_address("glBlitFramebuffer");
	tfx_glCopyImageSubData = get_proc_address("glCopyImageSubData");
	tfx_glFramebufferTexture2D = get_proc_address("glFramebufferTexture2D");
	tfx_glInvalidateFramebuffer = get_proc_address("glInvalidateFramebuffer");
	tfx_glGenRenderbuffers = get_proc_address("glGenRenderbuffers");
	tfx_glBindRenderbuffer = get_proc_address("glBindRenderbuffer");
	tfx_glRenderbufferStorage = get_proc_address("glRenderbufferStorage");
	tfx_glRenderbufferStorageMultisample = get_proc_address("glRenderbufferStorageMultisample");
	tfx_glFramebufferRenderbuffer = get_proc_address("glFramebufferRenderbuffer");
	tfx_glFramebufferTexture = get_proc_address("glFramebufferTexture");
	tfx_glDrawBuffers = get_proc_address("glDrawBuffers");
	tfx_glReadBuffer = get_proc_address("glReadBuffer");
	tfx_glCheckFramebufferStatus = get_proc_address("glCheckFramebufferStatus");
	tfx_glGetUniformLocation = get_proc_address("glGetUniformLocation");
	tfx_glReleaseShaderCompiler = get_proc_address("glReleaseShaderCompiler");
	tfx_glGenVertexArrays = get_proc_address("glGenVertexArrays");
	tfx_glBindVertexArray = get_proc_address("glBindVertexArray");
	tfx_glMapBufferRange = get_proc_address("glMapBufferRange");
	tfx_glBufferSubData = get_proc_address("glBufferSubData");
	tfx_glUnmapBuffer = get_proc_address("glUnmapBuffer");
	tfx_glUseProgram = get_proc_address("glUseProgram");
	tfx_glMemoryBarrier = get_proc_address("glMemoryBarrier");
	tfx_glBindBufferBase = get_proc_address("glBindBufferBase");
	tfx_glDispatchCompute = get_proc_address("glDispatchCompute");
	tfx_glViewport = get_proc_address("glViewport");
	tfx_glViewportIndexedf = get_proc_address("glViewportIndexedf");
	tfx_glScissor = get_proc_address("glScissor");
	tfx_glClearColor = get_proc_address("glClearColor");
	tfx_glClearDepthf = get_proc_address("glClearDepthf");
	tfx_glClear = get_proc_address("glClear");
	tfx_glEnable = get_proc_address("glEnable");
	tfx_glDepthFunc = get_proc_address("glDepthFunc");
	tfx_glDisable = get_proc_address("glDisable");
	tfx_glDepthMask = get_proc_address("glDepthMask");
	tfx_glFrontFace = get_proc_address("glFrontFace");
	tfx_glPolygonMode = get_proc_address("glPolygonMode");
	tfx_glUniform1iv = get_proc_address("glUniform1iv");
	tfx_glUniform1fv = get_proc_address("glUniform1fv");
	tfx_glUniform2fv = get_proc_address("glUniform2fv");
	tfx_glUniform3fv = get_proc_address("glUniform3fv");
	tfx_glUniform4fv = get_proc_address("glUniform4fv");
	tfx_glUniformMatrix2fv = get_proc_address("glUniformMatrix2fv");
	tfx_glUniformMatrix3fv = get_proc_address("glUniformMatrix3fv");
	tfx_glUniformMatrix4fv = get_proc_address("glUniformMatrix4fv");
	tfx_glEnableVertexAttribArray = get_proc_address("glEnableVertexAttribArray");
	tfx_glVertexAttribPointer = get_proc_address("glVertexAttribPointer");
	tfx_glDisableVertexAttribArray = get_proc_address("glDisableVertexAttribArray");
	tfx_glActiveTexture = get_proc_address("glActiveTexture");
	tfx_glDrawElementsInstanced = get_proc_address("glDrawElementsInstanced");
	tfx_glDrawArraysInstanced = get_proc_address("glDrawArraysInstanced");
	tfx_glDrawElements = get_proc_address("glDrawElements");
	tfx_glDrawArrays = get_proc_address("glDrawArrays");
	tfx_glDeleteVertexArrays = get_proc_address("glDeleteVertexArrays");
	tfx_glBindFragDataLocation = get_proc_address("glBindFragDataLocation");

	tfx_glPushDebugGroup = get_proc_address("glPushDebugGroup");
	tfx_glPopDebugGroup = get_proc_address("glPopDebugGroup");
	tfx_glInsertEventMarkerEXT = get_proc_address("glInsertEventMarkerEXT");
}

static const char *tfx_sprintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *buf = NULL;
	int need = vsnprintf(buf, 0, fmt, args) + 1;
	va_end(args);
	va_start(args, fmt);
	buf = malloc(need + 1);
	buf[need] = '\0';
	vsnprintf(buf, need, fmt, args);
	va_end(args);
	return buf;
}

static void tfx_printf(tfx_severity severity, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *buf = NULL;
	int need = vsnprintf(buf, 0, fmt, args) + 1;
	va_end(args);
	va_start(args, fmt);
	buf = malloc(need + 1);
	buf[need] = '\0';
	vsnprintf(buf, need, fmt, args);
	g_platform_data.info_log(buf, severity);
	free(buf);
	va_end(args);
}

static void tfx_printb(tfx_severity severity, const char *k, bool v) {
	tfx_printf(severity, "TinyFX %s: %s", k, v ? "true" : "false");
}

static float g_max_aniso = 0.0f;
tfx_caps tfx_get_caps() {
	tfx_caps caps;
	memset(&caps, 0, sizeof(tfx_caps));

	if (tfx_glGetStringi) {
		GLint ext_count = 0;
		CHECK(tfx_glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count));

		for (int i = 0; i < ext_count; i++) {
			const char *search = (const char*)CHECK(tfx_glGetStringi(GL_EXTENSIONS, i));
#if defined(_MSC_VER) && 0
			OutputDebugString(search);
			OutputDebugString("\n");
#endif
			for (int j = 0;; j++) {
				tfx_glext *tmp = &available_exts[j];
				if (!tmp->ext) {
					break;
				}
				if (strcmp(tmp->ext, search) == 0) {
					tmp->supported = true;
					break;
				}
			}
		}
	}
	else if (tfx_glGetString) {
		const char *real = (const char*)CHECK(tfx_glGetString(GL_EXTENSIONS));
		char *exts = tfx_strdup(real);
		char *pch = strtok(exts, " ");
		int len = 0;
		char **supported = NULL;
		while (pch != NULL) {
			sb_push(supported, pch);
			pch = strtok(NULL, " ");
			len++;
		}
		free(exts);

		int n = sb_count(supported);
		for (int i = 0; i < n; i++) {
			const char *search = supported[i];

			for (int j = 0; ; j++) {
				tfx_glext *tmp = &available_exts[j];
				if (!tmp->ext) {
					break;
				}
				if (strcmp(tmp->ext, search) == 0) {
					tmp->supported = true;
					break;
				}
			}
		}
		sb_free(supported);
	}

	bool gl30 = g_platform_data.context_version >= 30 && !g_platform_data.use_gles;
	bool gl32 = g_platform_data.context_version >= 32 && !g_platform_data.use_gles;
	bool gl33 = g_platform_data.context_version >= 33 && !g_platform_data.use_gles;
	bool gl43 = g_platform_data.context_version >= 43 && !g_platform_data.use_gles;
	bool gl44 = g_platform_data.context_version >= 44 && !g_platform_data.use_gles;
	bool gl46 = g_platform_data.context_version >= 46 && !g_platform_data.use_gles;
	bool gles30 = g_platform_data.context_version >= 30 && g_platform_data.use_gles;
	bool gles31 = g_platform_data.context_version >= 31 && g_platform_data.use_gles;

	caps.multisample = available_exts[0].supported || gl30;
	caps.compute = available_exts[1].supported || gles31 || gl43;
	caps.float_canvas = available_exts[2].supported || gles30 || gl30;
	caps.debug_marker = available_exts[3].supported || available_exts[5].supported;
	caps.debug_output = available_exts[4].supported || gl43;
	caps.memory_info = available_exts[6].supported;
	caps.instancing = available_exts[7].supported || gl33 || gles30;
	caps.seamless_cubemap = available_exts[8].supported || gl32;
	caps.anisotropic_filtering = available_exts[9].supported || gl46;
	caps.multibind = available_exts[10].supported || gl44;

	g_max_aniso = 0.0f;
	GLenum GL_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE;
	if (caps.anisotropic_filtering) {
		GLenum GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF;
		CHECK(tfx_glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &g_max_aniso));
		if (g_max_aniso <= 0.0f) {
			caps.anisotropic_filtering = false;
		}
	}

	return caps;
}

void tfx_dump_caps() {
	tfx_caps caps = tfx_get_caps();

	// I am told by the docs that this can be 0.
	// It's not on the RPi, but since it's only a few lines of code...
	int release_shader_c = 0;
	tfx_glGetIntegerv(GL_SHADER_COMPILER, &release_shader_c);
	TFX_INFO("GL shader compiler control: %d", release_shader_c);
	TFX_INFO("GL vendor: %s", tfx_glGetString(GL_VENDOR));
	TFX_INFO("GL version: %s", tfx_glGetString(GL_VERSION));

	TFX_INFO("%s", "GL extensions:");
	if (tfx_glGetStringi) {
		GLint ext_count = 0;
		CHECK(tfx_glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count));

		for (int i = 0; i < ext_count; i++) {
			const char *real = (const char*)CHECK(tfx_glGetStringi(GL_EXTENSIONS, i));
			TFX_INFO("\t%s", real);
		}
	}
	else if (tfx_glGetString) {
		const char *real = (const char*)CHECK(tfx_glGetString(GL_EXTENSIONS));
		char *exts = tfx_strdup(real);
		int len = 0;

		char *pch = strtok(exts, " ");
		while (pch != NULL) {
			TFX_INFO("\t%s", pch);
			pch = strtok(NULL, " ");
			len++;
		}
		free(exts);
	}

	#define FUG(V) "GLES" #V
	const char *glver =
	#ifdef TFX_USE_GLES // NYI
		FUG(TFX_USE_GLES/10)
	#else
		"GL4"
	#endif
	;
	#undef FUG
	TFX_INFO("TinyFX renderer: %s", glver);

	tfx_printb(TFX_SEVERITY_INFO, "instancing", caps.instancing);
	tfx_printb(TFX_SEVERITY_INFO, "compute", caps.compute);
	tfx_printb(TFX_SEVERITY_INFO, "fp canvas", caps.float_canvas);
	tfx_printb(TFX_SEVERITY_INFO, "multisample", caps.multisample);
}

// this is all definitely not the simplest way to deal with maps for uniform
// caches, but it's the simplest way I know which handles collisions.
#define TFX_HASHSIZE 101

typedef struct tfx_set {
	struct tfx_set *next;
	const char *key;
} tfx_set;

static unsigned tfx_hash(const char *s) {
	unsigned hashval;
	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % TFX_HASHSIZE;
}

static unsigned tfx_nohash(unsigned id) {
	return id % TFX_HASHSIZE;
}

static bool tfx_slookup(tfx_set **hashtab, const char *s) {
#ifdef TFX_DEBUG
	assert(s);
#endif
	struct tfx_set *np;
	for (np = hashtab[tfx_hash(s)]; np != NULL; np = np->next) {
		if (strcmp(s, np->key) == 0) {
			return true;
		}
		//TFX_WARN("collision\n");
	}
	return false;
}

static void tfx_sset(tfx_set **hashtab, const char *name) {
	if (tfx_slookup(hashtab, name)) {
		return;
	}

	unsigned hashval = tfx_hash(name);
	tfx_set *np = malloc(sizeof(tfx_set));
	np->key = name;
	np->next = hashtab[hashval];
	hashtab[hashval] = np;
}

static tfx_set **tfx_set_new() {
	return calloc(sizeof(tfx_set*), TFX_HASHSIZE);
}

static void tfx_set_delete(tfx_set **hashtab) {
	for (int i = 0; i < TFX_HASHSIZE; i++) {
		if (hashtab[i] != NULL) {
			free(hashtab[i]);
		}
	}
	free(hashtab);
}

typedef struct tfx_locmap {
	struct tfx_locmap *next;
	const char *key;
	GLuint value; // uniform location
} tfx_locmap;

static tfx_locmap *tfx_loclookup(tfx_locmap **hashtab, const char *s) {
#ifdef TFX_DEBUG
	assert(s);
#endif
	struct tfx_locmap *np;
	for (np = hashtab[tfx_hash(s)]; np != NULL; np = np->next) {
		if (strcmp(s, np->key) == 0) {
			return np;
		}
		//TFX_WARN("collision\n");
	}
	return NULL;
}

static tfx_locmap* tfx_locset(tfx_locmap **hashtab, const char *name, GLuint value) {
	tfx_locmap *found = tfx_loclookup(hashtab, name);
	if (found) {
		return NULL;
	}

	unsigned hashval = tfx_hash(name);
	tfx_locmap *np = malloc(sizeof(tfx_locmap));
	np->key = name;
	np->next = hashtab[hashval];
	np->value = value;
	hashtab[hashval] = np;
	return np;
}

static tfx_locmap **tfx_locmap_new() {
	return calloc(sizeof(tfx_locmap*), TFX_HASHSIZE);
}

static void tfx_locmap_delete(tfx_locmap **hashtab) {
	for (int i = 0; i < TFX_HASHSIZE; i++) {
		if (hashtab[i] != NULL) {
			free(hashtab[i]);
		}
	}
	free(hashtab);
}

typedef struct tfx_shadermap {
	struct tfx_shadermap *next;
	GLint key; // shader program
	tfx_locmap **value;
} tfx_shadermap;

static tfx_shadermap *tfx_proglookup(tfx_shadermap **hashtab, GLint program) {
#ifdef TFX_DEBUG
	assert(program);
#endif
	struct tfx_shadermap *np;
	for (np = hashtab[tfx_nohash(program)]; np != NULL; np = np->next) {
		if (program == np->key) {
			return np;
		}
		//TFX_WARN("collision\n");
	}
	return NULL;
}

static tfx_shadermap* tfx_progset(tfx_shadermap **hashtab, GLint program) {
	tfx_shadermap *found = tfx_proglookup(hashtab, program);
	if (found) {
		return found;
	}

	unsigned hashval = tfx_nohash(program);
	tfx_shadermap *np = malloc(sizeof(tfx_shadermap));
	np->key = program;
	np->next = hashtab[hashval];
	np->value = tfx_locmap_new();
	hashtab[hashval] = np;
	return np;
}

static tfx_shadermap **tfx_progmap_new() {
	return calloc(sizeof(tfx_shadermap*), TFX_HASHSIZE);
}

static void tfx_progmap_delete(tfx_shadermap **hashtab) {
	for (int i = 0; i < TFX_HASHSIZE; i++) {
		if (hashtab[i] != NULL) {
			tfx_locmap_delete(hashtab[i]->value);
			free(hashtab[i]);
		}
	}
	free(hashtab);
}

static tfx_buffer *g_buffers;

typedef struct tfx_frame_state {
	// uniforms updated this frame
	tfx_uniform *uniforms;
	uint8_t *uniform_buffer;
	uint8_t *ub_cursor;
	tfx_shadermap **uniform_map;
	tfx_view views[VIEW_MAX];
} tfx_frame_state;

static tfx_frame_state g_back;  // staging update
/*
// TODO
static tfx_frame_state g_front; // processing update

void swap_state_buffer() {
	tfx_frame_state tmp;
	memcpy(&tmp, &g_back, sizeof(tfx_frame_state));

	g_back = g_front;
	g_front = tmp;
}
*/

static struct {
	uint8_t *data;
	uint32_t offset;
	tfx_buffer buffers[TFX_TRANSIENT_BUFFER_COUNT];
} g_transient_buffer;

static tfx_caps g_caps;

// fallback printf
static void basic_log(const char *msg, tfx_severity level) {
#ifdef _MSC_VER
	OutputDebugString(msg);
	OutputDebugString("\n");
	printf("%s\n", msg);
#else
	printf("%s\n", msg);
#endif
}

static void tvb_reset() {
	g_transient_buffer.offset = 0;

	for (int i = 0; i < TFX_TRANSIENT_BUFFER_COUNT; i++) {
		if (!g_transient_buffer.buffers[i].gl_id) {
			GLuint id;
			CHECK(tfx_glGenBuffers(1, &id));
			CHECK(tfx_glBindBuffer(GL_ARRAY_BUFFER, id));
			if (tfx_glBufferStorage) {
				CHECK(tfx_glBufferStorage(GL_ARRAY_BUFFER, TFX_TRANSIENT_BUFFER_SIZE, NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT));
			}
			else {
				CHECK(tfx_glBufferData(GL_ARRAY_BUFFER, TFX_TRANSIENT_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW));
			}
			g_transient_buffer.buffers[i].gl_id = id;
		}
	}
}

void tfx_set_platform_data(tfx_platform_data pd) {
	// supported: GL > 3, 2.1, ES 2.0, ES 3.0+
	assert(0
		|| (pd.context_version >= 30)
		|| (pd.use_gles && pd.context_version == 20)
		|| (!pd.use_gles && pd.context_version == 21)
	);
	if (pd.info_log == NULL) {
		pd.info_log = &basic_log;
	}
	memcpy(&g_platform_data, &pd, sizeof(tfx_platform_data));
}

// null format = index buffer
tfx_transient_buffer tfx_transient_buffer_new(tfx_vertex_format *fmt, uint16_t num_verts) {
	// transient index buffers aren't supported yet
	assert(fmt != NULL);
	assert(fmt->stride > 0);

	tfx_transient_buffer buf;
	memset(&buf, 0, sizeof(tfx_transient_buffer));
	buf.data = g_transient_buffer.data + g_transient_buffer.offset;
	buf.num = num_verts;
	buf.offset = g_transient_buffer.offset;
	uint32_t stride = sizeof(uint16_t);
	if (fmt) {
		buf.has_format = true;
		buf.format = *fmt;
		stride = (uint32_t)fmt->stride;
	}
	g_transient_buffer.offset += (uint32_t)(num_verts * stride);
	g_transient_buffer.offset += g_transient_buffer.offset % 4; // align, in case the stride is weird
	return buf;
}

// null format = available indices (uint16)
uint32_t tfx_transient_buffer_get_available(tfx_vertex_format *fmt) {
	assert(fmt->stride > 0);
	uint32_t avail = TFX_TRANSIENT_BUFFER_SIZE;
	avail -= g_transient_buffer.offset;
	uint32_t stride = sizeof(uint16_t);
	if (fmt) {
		stride = fmt->stride;
	}
	avail /= (uint32_t)stride;
	return avail;
}

static tfx_program *g_programs = NULL;
static tfx_texture *g_textures = NULL;
static tfx_reset_flags g_flags = TFX_RESET_NONE;
static GLuint g_timers[TIMER_COUNT];
static int g_timer_offset = 0;
static bool use_timers = false;

static uint32_t *g_debug_data = NULL;
static tfx_program g_debug_program = 0;
static tfx_texture g_debug_overlay;
static tfx_uniform g_debug_texture;

static const char *g_debug_fss_legacy =
	"in vec2 f_coord;\n"
	"uniform sampler2D _tfx_texture;\n"
	"void main() {\n"
	"	vec4 color = texture2D(_tfx_texture, vec2(f_coord.x, 1.0 - f_coord.y));\n"
	"	if (color.a < 0.01) discard;\n"
	"	gl_FragColor = vec4(color.rgb, 1.0) * color.a;\n"
	"}\n"
;

static const char *g_debug_vss =
	"in vec3 v_position;\n"
	"out vec2 f_coord;\n"
	"void main() {\n"
	"	f_coord = v_position.xy * 0.5 + 0.5;\n"
	"	gl_Position = vec4(v_position.xyz, 1.0);\n"
	"}\n"
;

static const char *g_debug_fss =
	"in vec2 f_coord;\n"
	"out vec4 out_color;\n"
	"uniform sampler2D _tfx_texture;\n"
	"void main() {\n"
	"	vec4 color = texture(_tfx_texture, vec2(f_coord.x, 1.0 - f_coord.y));\n"
	"	if (color.a < 0.01) discard;\n"
	"	out_color = vec4(color.rgb, 1.0) * color.a;\n"
	"}\n"
;

static const char *g_debug_attribs[] = { "v_position", NULL };
static bool did_you_call_tfx_reset = false;

void tfx_reset(uint16_t width, uint16_t height, tfx_reset_flags flags) {
	if (g_platform_data.gl_get_proc_address != NULL) {
		load_em_up(g_platform_data.gl_get_proc_address);
	}

	did_you_call_tfx_reset = true;

	g_caps = tfx_get_caps();
	// we require these, unless/until backporting for pre-compute HW.
	//assert(g_caps.instancing);
	//assert(g_caps.compute);

	g_flags = TFX_RESET_NONE;
	if (g_caps.anisotropic_filtering && (flags & TFX_RESET_MAX_ANISOTROPY) == TFX_RESET_MAX_ANISOTROPY) {
		g_flags |= TFX_RESET_MAX_ANISOTROPY;
	}

	use_timers = false;
	if (tfx_glQueryCounter && (flags & TFX_RESET_REPORT_GPU_TIMINGS) == TFX_RESET_REPORT_GPU_TIMINGS) {
		g_flags |= TFX_RESET_REPORT_GPU_TIMINGS;
		use_timers = true;
	}

	if (g_debug_data != NULL) {
		free(g_debug_data);
		g_debug_data = NULL;
	}
	if (g_debug_overlay.gl_count > 0) {
		tfx_texture_free(&g_debug_overlay);
	}

	if ((flags & TFX_RESET_DEBUG_OVERLAY) == TFX_RESET_DEBUG_OVERLAY) {
		if (g_debug_program == 0) {
			if (g_platform_data.context_version < 30) {
				g_debug_program = tfx_program_new(g_debug_vss, g_debug_fss_legacy, g_debug_attribs, -1);
			}
			else {
				g_debug_program = tfx_program_new(g_debug_vss, g_debug_fss, g_debug_attribs, -1);
			}
			assert(g_debug_program);
		}
		if (g_debug_texture.name == NULL) {
			g_debug_texture = tfx_uniform_new("_tfx_texture", TFX_UNIFORM_INT, 1);
		}
		g_flags |= TFX_RESET_DEBUG_OVERLAY;

		if ((flags & TFX_RESET_DEBUG_OVERLAY_STATS) == TFX_RESET_DEBUG_OVERLAY_STATS) {
			g_flags |= TFX_RESET_DEBUG_OVERLAY_STATS;
		}

		// align texture size up to nearest character size
		uint16_t ow = (uint16_t)width;
		ow += ow % 8;
		uint16_t oh = (uint16_t)height;
		oh += oh % 8;
		size_t mem = (size_t)ow * oh * 4;
		g_debug_data = malloc(mem);
		g_debug_overlay = tfx_texture_new(ow, oh, 1, NULL, TFX_FORMAT_RGBA8, TFX_TEXTURE_CPU_WRITABLE | TFX_TEXTURE_FILTER_POINT);
	}

	memset(&g_backbuffer, 0, sizeof(tfx_canvas));
	//g_backbuffer.gl_fbo = g_platform_data.backbuffer_fbo;
	g_backbuffer.allocated = 1;
	g_backbuffer.width = width;
	g_backbuffer.height = height;
	g_backbuffer.current_width = width;
	g_backbuffer.current_height = height;
	g_backbuffer.attachments[0].width = width;
	g_backbuffer.attachments[0].height = height;
	g_backbuffer.attachments[0].depth = 1;

	if (!g_back.uniform_buffer) {
		g_back.uniform_buffer = (uint8_t*)malloc(TFX_UNIFORM_BUFFER_SIZE);
		memset(g_back.uniform_buffer, 0, TFX_UNIFORM_BUFFER_SIZE);
		g_back.ub_cursor = g_back.uniform_buffer;
	}

	if (!g_transient_buffer.data) {
		g_transient_buffer.data = (uint8_t*)malloc(TFX_TRANSIENT_BUFFER_SIZE);
		memset(g_transient_buffer.data, 0xfc, TFX_TRANSIENT_BUFFER_SIZE);
		tvb_reset();
	}

	if (!g_back.uniform_map) {
		g_back.uniform_map = tfx_progmap_new();
	}

	// update every already loaded texture's anisotropy to max (typically 16) or 0
	if (g_caps.anisotropic_filtering) {
		int nt = sb_count(g_textures);
		if (g_max_aniso > 0.0f) {
			for (int i = 0; i < nt; i++) {
				tfx_texture *tex = &g_textures[i];
				for (unsigned j = 0; j < tex->gl_count; j++) {
					if ((tex->flags & TFX_TEXTURE_MSAA_SAMPLE) == TFX_TEXTURE_MSAA_SAMPLE) {
						continue;
					}
					GLenum fmt = GL_TEXTURE_2D;
					if ((tex->flags & TFX_TEXTURE_CUBE) == TFX_TEXTURE_CUBE) {
						fmt = GL_TEXTURE_CUBE_MAP;
					}
					const GLenum GL_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE;
					CHECK(tfx_glBindTexture(fmt, tex->gl_ids[j]));
					CHECK(tfx_glTexParameterf(fmt, GL_TEXTURE_MAX_ANISOTROPY_EXT, g_max_aniso));
				}
			}
		}
	}

#if defined(_MSC_VER) && defined(TFX_DEBUG)
	if (g_caps.memory_info) {
		GLint memory;
		// GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    
		CHECK(tfx_glGetIntegerv(0x9048, &memory));
		char buf[64];
		snprintf(buf, 64, "VRAM: %dMiB\n", memory / 1024);
		OutputDebugString(buf);
	}
#endif

	memset(&g_back.views, 0, sizeof(tfx_view)*VIEW_MAX);

	// not supported in ES2 w/o exts
	if (tfx_glQueryCounter) {
		if (g_timers[0] != 0) {
			CHECK(tfx_glDeleteQueries(TIMER_COUNT, g_timers));
		}
	}
	if (use_timers) {
		CHECK(tfx_glGenQueries(TIMER_COUNT, g_timers));
		// dummy queries for first update
		for (unsigned i = 0; i < TIMER_COUNT; i++) {
			CHECK(tfx_glQueryCounter(g_timers[i], GL_TIMESTAMP));
		}
		g_timer_offset = 0;
	}
}

void tfx_shutdown() {
	tfx_frame();

	if (tfx_glQueryCounter && g_timers[0] != 0) {
		CHECK(tfx_glDeleteQueries(TIMER_COUNT, g_timers));
	}

	// TODO: clean up all GL objects, allocs, etc.
	free(g_back.uniform_buffer);
	g_back.uniform_buffer = NULL;

	free(g_transient_buffer.data);
	g_transient_buffer.data = NULL;

	for (int i = 0; i < TFX_TRANSIENT_BUFFER_COUNT; i++) {
		if (g_transient_buffer.buffers[i].gl_id) {
			tfx_glDeleteBuffers(1, &g_transient_buffer.buffers[i].gl_id);
		}
	}

	if (g_back.uniform_map) {
		tfx_progmap_delete(g_back.uniform_map);
		g_back.uniform_map = NULL;
	}

	// this can happen if you shutdown before calling frame()
	if (g_back.uniforms) {
		sb_free(g_back.uniforms);
		g_back.uniforms = NULL;
	}

	int nt = sb_count(g_textures);
	while (nt-- > 0) {
		tfx_texture_free(&g_textures[nt]);
	}
	sb_free(g_textures);

	int nb = sb_count(g_buffers);
	while (nb-- > 0) {
		tfx_buffer_free(&g_buffers[nb]);
	}
	sb_free(g_buffers);

	tfx_glUseProgram(0);
	int np = sb_count(g_programs);
	for (int i = 0; i < np; i++) {
		tfx_glDeleteProgram(g_programs[i]);
	}
	sb_free(g_programs);

#ifdef TFX_LEAK_CHECK
	stb_leakcheck_dumpmem();
#endif
}

static bool g_shaderc_allocated = false;
static int stack_depth = 0;

static void push_group(unsigned id, const char *label) {
	if (tfx_glPushDebugGroup) {
		CHECK(tfx_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, strlen(label), label));
		stack_depth += 1;
	}
}

static void pop_group() {
	if (tfx_glPopDebugGroup && stack_depth > 0) {
		CHECK(tfx_glPopDebugGroup());
		stack_depth -= 1;
	}
}

const char *legacy_vs_prepend = ""
	"#ifndef GL_ES\n"
	"#define lowp\n"
	"#define mediump\n"
	"#define highp\n"
	"#else\n"
	"precision highp float;\n"
	"#define in attribute\n"
	"#define out varying\n"
	"#endif\n"
	"#pragma optionNV(strict on)\n"
	"#define main _pain\n"
	"#define tfx_viewport_count 1\n"
	"#define VERTEX 1\n"
	"#line 1\n"
;
const char *legacy_fs_prepend = ""
	"#ifndef GL_ES\n"
	"#define lowp\n"
	"#define mediump\n"
	"#define highp\n"
	"#else\n"
	"precision mediump float;\n"
	"#define in varying\n"
	"#endif\n"
	"#pragma optionNV(strict on)\n"
	"#define PIXEL 1\n"
	"#line 1\n"
;
const char *gs_prepend = ""
	"#ifdef GL_ES\n"
	"precision highp float;\n"
	"#endif\n"
	"#define GEOMETRY 1\n"
	"#line 1\n"
;
const char *vs_prepend = ""
	"#ifdef GL_ES\n"
	"precision highp float;\n"
	"#endif\n"
	"#define main _pain\n"
	"#define tfx_viewport_count 1\n"
	"#define VERTEX 1\n"
	"#line 1\n"
;
const char *fs_prepend = ""
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"#define PIXEL 1\n"
	"#line 1\n"
;

const char *vs_append = ""
	"#undef main\n"
	"void main() {\n"
	"	_pain();\n"
	// TODO: enable extensions if GL < 4.1, add gl_Layer support for
	// single pass cube/shadow cascade rendering
	"#if 0\n"
	"	gl_ViewportIndex = gl_InstanceID % tfx_viewport_count;\n"
	"#endif\n"
	"}\n"
;

const char *cs_prepend = ""
	"#define COMPUTE 1\n"
	"#line 1\n"
;

static char *sappend(const char *left, const char *right, const int right_len) {
	size_t ls = strlen(left);
	size_t rs = (size_t)right_len;
	char *ss = (char*)malloc(ls+rs+1);
	memcpy(ss, left, ls);
	memcpy(ss+ls, right, rs);
	ss[ls+rs] = '\0';
	return ss;
}

static char *shader_concat(const char *base, GLenum shader_type, const int base_len) {
	bool legacy = g_platform_data.context_version < 30;

	const char *prepend = "";
	const char *append = "";

	switch (shader_type) {
		case GL_COMPUTE_SHADER: {
			assert(g_caps.compute);
			prepend = cs_prepend;
			break;
		}
		case GL_VERTEX_SHADER: {
			prepend = legacy ? legacy_vs_prepend : vs_prepend;
			append = vs_append;
			break;
		}
		case GL_FRAGMENT_SHADER: {
			prepend = legacy ? legacy_fs_prepend: fs_prepend;
			break;
		}
		case GL_GEOMETRY_SHADER: {
			prepend = gs_prepend;
			break;
		}
		default: break;
	}

	char *ss1 = sappend(prepend, base, base_len);

	char version[64];
	int gl_major = g_platform_data.context_version / 10;
	int gl_minor = g_platform_data.context_version % 10;
	const char *suffix = " core";
	// post GL3/GLES3, versions are sane, just fix suffix.
	if (g_platform_data.use_gles && g_platform_data.context_version >= 30) {
		suffix = " es";
	}
	// GL and GLES2 use GLSL 1xx versions
	else if (g_platform_data.context_version < 30) {
		suffix = "";
		gl_major = 1;
		// GL 2.1 -> GLSL 120
		if (!g_platform_data.use_gles) {
			gl_minor = 2;
		}
	}
	snprintf(version, 64, "#version %d%d0%s\n", gl_major, gl_minor, suffix);

	char *ss2 = sappend(ss1, append, strlen(append));
	free(ss1);

	char *ss = sappend(version, ss2, strlen(ss2));
	free(ss2);

	return ss;
}

static GLuint load_shader(GLenum type, const char *shaderSrc, const int len) {
	g_shaderc_allocated = true;

	GLuint shader = CHECK(tfx_glCreateShader(type));
	if (!shader) {
		TFX_FATAL("%s", "Something has gone horribly wrong, and we can't make shaders.");
		return 0;
	}

	if (len <= 0) {
		return 0;
	}

	char *ss = shader_concat(shaderSrc, type, len);
	CHECK(tfx_glShaderSource(shader, 1, (const char**)&ss, NULL));
	CHECK(tfx_glCompileShader(shader));

	GLint compiled;
	CHECK(tfx_glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
	if (!compiled) {
		GLint infoLen = 0;
		CHECK(tfx_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen));
		if (infoLen > 0) {
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);
			CHECK(tfx_glGetShaderInfoLog(shader, infoLen, NULL, infoLog));
			TFX_ERROR("Error compiling shader:\n%s", infoLog);
			// TFX_ERROR("FULL SHADER\n\n\n%s\n\n\n", ss);
#ifdef _MSC_VER
			OutputDebugString(infoLog);
#endif
			free(infoLog);
		}
		CHECK(tfx_glDeleteShader(shader));
#ifdef TFX_DEBUG
		assert(compiled);
#endif
		free(ss);
		return 0;
	}
	// free this a bit late to make debugging easier
	free(ss);
	return shader;
}

static bool try_program_link(GLuint program) {
	CHECK(tfx_glLinkProgram(program));
	GLint linked;
	CHECK(tfx_glGetProgramiv(program, GL_LINK_STATUS, &linked));
	if (!linked) {
		GLint infoLen = 0;
		CHECK(tfx_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen));
		if (infoLen > 0) {
			char* infoLog = (char*)malloc(infoLen);
			CHECK(tfx_glGetProgramInfoLog(program, infoLen, NULL, infoLog));
			TFX_ERROR("Error linking program:\n%s", infoLog);
			free(infoLog);
		}
		return false;
	}
	return true;
}

tfx_program tfx_program_gs_len_new(const char *_gss, const int _gs_len, const char *_vss, const int _vs_len, const char *_fss, const int _fs_len, const char *attribs[], const int attrib_count) {
	assert(did_you_call_tfx_reset);

	GLuint gs = 0;
	if (_gss) {
		gs = load_shader(GL_GEOMETRY_SHADER, _gss, _gs_len);
	}
	GLuint vs = 0;
	if (_vss) {
		vs = load_shader(GL_VERTEX_SHADER, _vss, _vs_len);
	}
	GLuint fs = 0;
	if (_fss) {
		fs = load_shader(GL_FRAGMENT_SHADER, _fss, _fs_len);
	}
	GLuint program = CHECK(tfx_glCreateProgram());
	if (!program) {
		return 0;
	}

	if (gs) {
		CHECK(tfx_glAttachShader(program, gs));
	}
	if (vs) {
		CHECK(tfx_glAttachShader(program, vs));
	}
	if (fs) {
		CHECK(tfx_glAttachShader(program, fs));
	}

	if (attrib_count >= 0) {
		for (int i = 0; i < attrib_count; i++) {
			CHECK(tfx_glBindAttribLocation(program, i, attribs[i]));
		}
	}
	else {
		const char **it = attribs;
		int i = 0;
		while (*it != NULL) {
			CHECK(tfx_glBindAttribLocation(program, i, *it));
			i++;
			it++;
		}
	}

	// TODO: accept frag data binding array
	if (tfx_glBindFragDataLocation) {
		//CHECK(tfx_glBindFragDataLocation(program, 0, "out_color"));
	}

	if (!try_program_link(program)) {
		CHECK(tfx_glDeleteProgram(program));
		return 0;
	}

	if (gs) {
		CHECK(tfx_glDeleteShader(gs));
	}
	CHECK(tfx_glDeleteShader(vs));
	CHECK(tfx_glDeleteShader(fs));

	sb_push(g_programs, program);

	return program;
}

tfx_program tfx_program_gs_new(const char *_gss, const char *_vss, const char *_fss, const char *attribs[], const int attrib_count) {
	int gs_len = 0;
	int vs_len = 0;
	int fs_len = 0;
	if (_gss) {
		gs_len = strlen(_gss);
	}
	if (_vss) {
		vs_len = strlen(_vss);
	}
	if (_fss) {
		fs_len = strlen(_fss);
	}
	return tfx_program_gs_len_new(_gss, gs_len, _vss, vs_len, _fss, fs_len, attribs, attrib_count);
}

tfx_program tfx_program_cs_len_new(const char *css, const int cs_len) {
	assert(did_you_call_tfx_reset);

	if (!g_caps.compute) {
		return 0;
	}

	GLuint cs = load_shader(GL_COMPUTE_SHADER, css, cs_len);
	GLuint program = CHECK(tfx_glCreateProgram());
	if (!program) {
		return 0;
	}
	CHECK(tfx_glAttachShader(program, cs));
	if (!try_program_link(program)) {
		CHECK(tfx_glDeleteProgram(program));
		return 0;
	}
	CHECK(tfx_glDeleteShader(cs));

	sb_push(g_programs, program);

	return program;
}

tfx_program tfx_program_cs_new(const char *_css) {
	int cs_len = 0;
	if (_css) {
		cs_len = strlen(_css);
	}
	return tfx_program_cs_len_new(_css, cs_len);
}

tfx_program tfx_program_new(const char *_vss, const char *_fss, const char *attribs[], const int attrib_count) {
	return tfx_program_gs_new(NULL, _vss, _fss, attribs, attrib_count);
}

tfx_program tfx_program_len_new(const char *_vss, const int _vs_len, const char *_fss, const int _fs_len, const char *attribs[], const int attrib_count) {
	return tfx_program_gs_len_new(NULL, 0, _vss, _vs_len, _fss, _fs_len, attribs, attrib_count);
}

tfx_vertex_format tfx_vertex_format_start() {
	tfx_vertex_format fmt;
	memset(&fmt, 0, sizeof(tfx_vertex_format));

	return fmt;
}

void tfx_vertex_format_add(tfx_vertex_format *fmt, uint8_t slot, size_t count, bool normalized, tfx_component_type type) {
	assert(type >= 0 && type <= TFX_TYPE_SKIP);

	if (slot >= fmt->count) {
		fmt->count = slot + 1;
	}
	tfx_vertex_component *component = &fmt->components[slot];
	memset(component, 0, sizeof(tfx_vertex_component));
	component->offset = 0;
	component->size = count;
	component->normalized = normalized;
	component->type = type;

	fmt->component_mask |= 1 << slot;
}

size_t tfx_vertex_format_offset(tfx_vertex_format *fmt, uint8_t slot) {
	assert(slot < 8);
	return fmt->components[slot].offset;
}

void tfx_vertex_format_end(tfx_vertex_format *fmt) {
	size_t stride = 0;
	int nc = fmt->count;
	for (int i = 0; i < nc; i++) {
		tfx_vertex_component *vc = &fmt->components[i];
		size_t bytes = 0;
		switch (vc->type) {
			case TFX_TYPE_SKIP:
			case TFX_TYPE_UBYTE:
			case TFX_TYPE_BYTE: bytes = 1; break;
			case TFX_TYPE_USHORT:
			case TFX_TYPE_SHORT: bytes = 2; break;
			case TFX_TYPE_FLOAT: bytes = 4; break;
			default: assert(0); break;
		}
		vc->offset = stride;
		stride += vc->size * bytes;
	}
	fmt->stride = stride;
}

tfx_buffer tfx_buffer_new(const void *data, size_t size, tfx_vertex_format *format, tfx_buffer_flags flags) {
	assert(did_you_call_tfx_reset);

	GLenum gl_usage = GL_STATIC_DRAW;
	switch (flags) {
		case TFX_BUFFER_MUTABLE: gl_usage = GL_DYNAMIC_DRAW; break;
		//case TFX_BUFFER_STREAM:  gl_usage = GL_STREAM_DRAW; break;
		default: break;
		//default: assert(0); break;
	}

	tfx_buffer buffer;
	memset(&buffer, 0, sizeof(tfx_buffer));
	buffer.gl_id = 0;
	buffer.flags = flags;
	if (format) {
		assert(format->stride > 0);

		buffer.has_format = true;
		buffer.format = *format;
	}

	CHECK(tfx_glGenBuffers(1, &buffer.gl_id));
	CHECK(tfx_glBindBuffer(GL_ARRAY_BUFFER, buffer.gl_id));

	if (size != 0) {
		if (tfx_glBufferStorage) {
			CHECK(tfx_glBufferStorage(GL_ARRAY_BUFFER, size, data, (gl_usage == GL_DYNAMIC_DRAW ? (GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT) : 0)))
		}
		else {
			CHECK(tfx_glBufferData(GL_ARRAY_BUFFER, size, data, gl_usage));
		}
	}

	sb_push(g_buffers, buffer);

	return buffer;
}

typedef struct tfx_buffer_params {
	uint32_t offset;
	uint32_t size;
	const void *update_data;
} tfx_buffer_params;

static tfx_buffer *get_internal_buffer(tfx_buffer *buf) {
	int nb = sb_count(g_buffers);
	for (int i = 0; i < nb; i++) {
		if (g_buffers[i].gl_id == buf->gl_id) {
			return &g_buffers[i];
		}
	}
	return NULL;
}

void tfx_buffer_update(tfx_buffer *buf, const void *data, uint32_t offset, uint32_t size) {
	assert(buf != NULL);
	assert((buf->flags & TFX_BUFFER_MUTABLE) == TFX_BUFFER_MUTABLE);
	assert(size > 0);
	assert(data != NULL);
	tfx_buffer_params *params = buf->internal;
	if (!buf->internal) {
		params = calloc(1, sizeof(tfx_buffer_params));
	}
	params->update_data = data;
	params->offset = offset;
	params->size = size;
	get_internal_buffer(buf)->internal = params;
}

void tfx_buffer_free(tfx_buffer *buf) {
	CHECK(tfx_glDeleteBuffers(1, &buf->gl_id));
	if (buf->internal) {
		free(buf->internal);
		buf->internal = NULL;
	}
	int nb = sb_count(g_buffers);
	for (int i = 0; i < nb; i++) {
		tfx_buffer *cached = &g_buffers[i];
		// we only need to check index 0, as these ids cannot overlap or be reused.
		if (buf->gl_id == cached->gl_id) {
			g_buffers[i] = g_buffers[nb - 1];
			// this, uh, might not be right.
			stb__sbraw(g_buffers)[1] -= 1;
		}
	}
}

typedef struct tfx_texture_params {
	GLenum format;
	GLenum internal_format;
	GLenum type;
	const void *update_data;
} tfx_texture_params;

tfx_texture tfx_texture_new(uint16_t w, uint16_t h, uint16_t layers, const void *data, tfx_format format, uint16_t flags) {
	assert(did_you_call_tfx_reset);

	tfx_texture t;
	memset(&t, 0, sizeof(tfx_texture));

	t.width = w;
	t.height = h;
	t.depth = layers;
	t.format = format;
	t.flags = flags;

	t.gl_count = 1;

	bool msaa_sample = (flags & TFX_TEXTURE_MSAA_SAMPLE) == TFX_TEXTURE_MSAA_SAMPLE;

	// double buffer the texture updates, to reduce stalling.
	if ((flags & TFX_TEXTURE_CPU_WRITABLE) == TFX_TEXTURE_CPU_WRITABLE || msaa_sample) {
		t.gl_count = 2;
	}

	int samples = 1;
	if ((flags & TFX_TEXTURE_MSAA_X2) == TFX_TEXTURE_MSAA_X2) {
		assert(t.gl_count == 1 || msaa_sample);
		samples = 2;
	}
	if ((flags & TFX_TEXTURE_MSAA_X4) == TFX_TEXTURE_MSAA_X4) {
		assert(t.gl_count == 1 || msaa_sample);
		samples = 4;
	}

	t.gl_idx = 0;

	tfx_texture_params *params = calloc(1, sizeof(tfx_texture_params));
	params->update_data = NULL;

	// TODO: add some stencil formats (i.e. D24S8)
	bool stencil = false;
	bool depth = false;
	switch (format) {
		// integer formats
		case TFX_FORMAT_RGB565:
			params->format = GL_RGB;
			params->internal_format = GL_RGB565;
			params->type = GL_UNSIGNED_SHORT_5_6_5;
			break;
		case TFX_FORMAT_SRGB8:
			params->format = GL_RGB;
			params->internal_format = GL_SRGB8;
			params->type = GL_UNSIGNED_BYTE;
			break;
		case TFX_FORMAT_SRGB8_A8:
			params->format = GL_RGBA;
			params->internal_format = GL_SRGB8_ALPHA8;
			params->type = GL_UNSIGNED_BYTE;
			break;
		case TFX_FORMAT_RGBA8:
			params->format = GL_RGBA;
			params->internal_format = GL_RGBA8;
			params->type = GL_UNSIGNED_BYTE;
			break;
		case TFX_FORMAT_RGB10A2:
			params->format = GL_RGBA;
			params->internal_format = GL_RGB10_A2;
			params->type = GL_UNSIGNED_INT_10_10_10_2;
			break;
		case TFX_FORMAT_R32UI:
			params->format = GL_RED;
			params->internal_format = GL_R32UI;
			params->type = GL_UNSIGNED_INT;
			break;
		// float formats
		case TFX_FORMAT_RG11B10F:
			params->format = GL_RGB;
			params->internal_format = GL_R11F_G11F_B10F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_RGB16F:
			params->format = GL_RGB;
			params->internal_format = GL_RGB16F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_RGBA16F:
			params->format = GL_RGBA;
			params->internal_format = GL_RGBA16F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_R16F:
			params->format = GL_RED;
			params->internal_format = GL_R16F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_R32F:
			params->format = GL_RED;
			params->internal_format = GL_R32F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_RG16F:
			params->format = GL_RG;
			params->internal_format = GL_RG16F;
			params->type = GL_FLOAT;
			break;
		case TFX_FORMAT_RG32F:
			params->format = GL_RG;
			params->internal_format = GL_RG32F;
			params->type = GL_FLOAT;
			break;
		// depth formats
		case TFX_FORMAT_D16:
			params->format = GL_DEPTH_COMPONENT;
			params->internal_format = GL_DEPTH_COMPONENT16;
			params->type = GL_UNSIGNED_BYTE;
			depth = true;
			break;
		case TFX_FORMAT_D24:
			params->format = GL_DEPTH_COMPONENT;
			params->internal_format = GL_DEPTH_COMPONENT24;
			params->type = GL_UNSIGNED_BYTE;
			depth = true;
			break;
		case TFX_FORMAT_D32:
			params->format = GL_DEPTH_COMPONENT;
			params->internal_format = GL_DEPTH_COMPONENT32;
			params->type = GL_UNSIGNED_INT;
			depth = true;
			break;
		case TFX_FORMAT_D32F:
			params->format = GL_DEPTH_COMPONENT;
			params->internal_format = GL_DEPTH_COMPONENT32F;
			params->type = GL_FLOAT;
			depth = true;
			break;
		// invalid
		case TFX_FORMAT_RGB565_D16:
		case TFX_FORMAT_RGBA8_D16:
		case TFX_FORMAT_RGBA8_D24:
		default:
			assert(0);
			break;
	}
	t.is_stencil = stencil;
	t.is_depth = depth;
	t.internal = params;

	if (samples > 1 && t.gl_count == 1) {
		CHECK(tfx_glGenRenderbuffers(1, &t.gl_msaa_id));
		CHECK(tfx_glBindRenderbuffer(GL_RENDERBUFFER, t.gl_msaa_id));
		CHECK(tfx_glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, params->internal_format, w, h));
	}

	CHECK(tfx_glGenTextures(t.gl_count, t.gl_ids));
	bool aniso = (g_flags & TFX_RESET_MAX_ANISOTROPY) == TFX_RESET_MAX_ANISOTROPY;
	bool reserve_mips = (flags & TFX_TEXTURE_RESERVE_MIPS) == TFX_TEXTURE_RESERVE_MIPS;
	bool gen_mips = (flags & TFX_TEXTURE_GEN_MIPS) == TFX_TEXTURE_GEN_MIPS;
	bool cube     = (flags & TFX_TEXTURE_CUBE) == TFX_TEXTURE_CUBE;
	GLenum mode = 0;
	if (layers > 1) {
		if (cube) {
			assert(0); // only supported by GL4.0+ or with ARB_texture_cube_map_array
			mode = GL_TEXTURE_CUBE_MAP_ARRAY;
		}
		else {
			mode = GL_TEXTURE_2D_ARRAY;
		}
	}
	else {
		if (cube) {
			mode = GL_TEXTURE_CUBE_MAP;
		}
		else {
			mode = GL_TEXTURE_2D;
		}
	}

	bool mip_filter = reserve_mips || gen_mips;
	if (mip_filter) {
		assert(!msaa_sample);
		t.mip_count = 1 + (int)floorf(log2f(fmaxf(t.width, t.height)));
	}

	for (unsigned i = 0; i < t.gl_count; i++) {
		assert(t.gl_ids[i] > 0);

		bool msaa = msaa_sample && i == 1;
		if (msaa) {
			assert(layers == 1);
			assert(!cube);
			assert(!reserve_mips);
			mode = GL_TEXTURE_2D_MULTISAMPLE;
			CHECK(tfx_glBindTexture(mode, t.gl_ids[i]));
			if (tfx_glTexStorage2DMultisample) {
				CHECK(tfx_glTexStorage2DMultisample(mode, samples, params->internal_format, w, h, false));
			}
			else {
				CHECK(tfx_glTexImage2DMultisample(mode, samples, params->internal_format, w, h, false));
			}
			continue;
		}

		CHECK(tfx_glBindTexture(mode, t.gl_ids[i]));
		if ((flags & TFX_TEXTURE_FILTER_POINT) == TFX_TEXTURE_FILTER_POINT) {
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_MIN_FILTER, mip_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		}
		else { // default filter: linear
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_MIN_FILTER, mip_filter ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		}

		if (cube) {
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
		}
		CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		if (aniso) {
			GLenum GL_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE;
			CHECK(tfx_glTexParameterf(mode, GL_TEXTURE_MAX_ANISOTROPY_EXT, g_max_aniso));
		}

		// if mips are reserved, this isn't a shadow map but instead something like hi-z buffer. can't ref compare.
		if (depth && !reserve_mips) {
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));

			// note: GL 3.3, ES 3.0+. combined swizzle isn't in gles.
			GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_SWIZZLE_R, swizzleMask[0]));
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_SWIZZLE_G, swizzleMask[1]));
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_SWIZZLE_B, swizzleMask[2]));
			CHECK(tfx_glTexParameteri(mode, GL_TEXTURE_SWIZZLE_A, swizzleMask[3]));
		}
		CHECK(tfx_glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

		if (layers > 1) {
			if (cube) {
				assert(0);
			}
			else if (tfx_glTexStorage3D) {
				// mipmaps are currently unsupported for 3d/array textures.
				CHECK(tfx_glTexStorage3D(mode, 1, params->internal_format, w, h, layers));
				if (data) {
					CHECK(tfx_glTexSubImage3D(mode, 0, 0, 0, 0, w, h, layers, params->format, params->type, data));
				}
			}
			else {
				CHECK(tfx_glTexImage3D(mode, 0, params->internal_format, w, h, layers, 0, params->format, params->type, data));
			}
		}
		if (cube) {
			const uint16_t size = w > h ? w : h;
			if (tfx_glTexStorage2D) {
				CHECK(tfx_glTexStorage2D(mode, mip_filter ? t.mip_count : 1, params->internal_format, size, size));
			}
			else {
				uint16_t mip_size = size;
				for (i = 0; i < t.mip_count; i++) {
					for (int j = 0; j < 6; j++) {
						CHECK(tfx_glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, params->internal_format, size, size, 0, params->format, params->type, NULL));
					}
					mip_size /= 2;
					mip_size = mip_size > 0 ? mip_size : 1;
				}
			}
		}
		else if (tfx_glTexStorage2D) {
			CHECK(tfx_glTexStorage2D(mode, mip_filter ? t.mip_count : 1, params->internal_format, w, h));
			if (data) {
				CHECK(tfx_glTexSubImage2D(mode, 0, 0, 0, w, h, params->format, params->type, data));
			}
		}
		else {
			CHECK(tfx_glTexImage2D(mode, 0, params->internal_format, w, h, 0, params->format, params->type, data));
			if (reserve_mips) {
				uint16_t current_width = t.width;
				uint16_t current_height = t.height;

				for (unsigned i = 1; i < t.mip_count; i++) {
					// calculate next viewport size
					current_width /= 2;
					current_height /= 2;

					// ensure that the viewport size is always at least 1x1
					current_width = current_width > 0 ? current_width : 1;
					current_height = current_height > 0 ? current_height : 1;

					CHECK(tfx_glTexImage2D(mode, i, params->internal_format, current_width, current_height, 0, params->format, params->type, NULL));
				}
			}
		}
		if (gen_mips) {
			CHECK(tfx_glGenerateMipmap(mode));
		}
	}

	sb_push(g_textures, t);

	return t;
}

void tfx_texture_update(tfx_texture *tex, const void *data) {
	assert((tex->flags & TFX_TEXTURE_CPU_WRITABLE) == TFX_TEXTURE_CPU_WRITABLE);
	tfx_texture_params *internal = tex->internal;
	internal->update_data = data;
}

void tfx_texture_free(tfx_texture *tex) {
	int nt = sb_count(g_textures);
	for (int i = 0; i < nt; i++) {
		tfx_texture *cached = &g_textures[i];
		// we only need to check index 0, as these ids cannot overlap or be reused.
		if (tex->gl_ids[0] == cached->gl_ids[0]) {
			tfx_texture_params *internal = (tfx_texture_params*)cached->internal;
			free(internal);
			tfx_glDeleteTextures(cached->gl_count, cached->gl_ids);
			g_textures[i] = g_textures[nt-1];
			// this, uh, might not be right.
			stb__sbraw(g_textures)[1] -= 1;
		}
	}
}

bool canvas_reconfigure(tfx_canvas *c, bool msaa) {
	bool found_color = false;
	bool found_depth = false;

	int offset = 0;
	for (unsigned i = 0; i < c->allocated; i++) {
		GLenum attach = GL_COLOR_ATTACHMENT0 + offset;
		// TODO: depth stencil
		if (c->attachments[i].is_depth) {
			assert(!found_depth); // two depth buffers, bail
			attach = GL_DEPTH_ATTACHMENT;
			found_depth = true;
		}
		else {
			found_color = true;
			offset += 1;
		}

		if (c->cube || c->attachments[i].depth > 1) {
			CHECK(tfx_glFramebufferTexture(GL_FRAMEBUFFER, attach, c->attachments[i].gl_ids[0], 0));
			continue;
		}

		GLenum mode = GL_TEXTURE_2D;
		GLuint id = c->attachments[i].gl_ids[0];
		if (msaa && (c->attachments[i].flags & TFX_TEXTURE_MSAA_SAMPLE) == TFX_TEXTURE_MSAA_SAMPLE) {
			mode = GL_TEXTURE_2D_MULTISAMPLE;
			id = c->attachments[i].gl_ids[1];
		}
		CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, attach, mode, id, 0));
	}

	if (found_depth && !found_color) {
		GLenum none = GL_NONE;
		CHECK(tfx_glDrawBuffers(1, &none));
		CHECK(tfx_glReadBuffer(GL_NONE));
	}

	if (found_color) {
		GLenum buffers[8] = {
			GL_COLOR_ATTACHMENT0 + 0,
			GL_COLOR_ATTACHMENT0 + 1,
			GL_COLOR_ATTACHMENT0 + 2,
			GL_COLOR_ATTACHMENT0 + 3,
			GL_COLOR_ATTACHMENT0 + 4,
			GL_COLOR_ATTACHMENT0 + 5,
			GL_COLOR_ATTACHMENT0 + 6,
			GL_COLOR_ATTACHMENT0 + 7
		};
		CHECK(tfx_glDrawBuffers(offset, buffers));
		CHECK(tfx_glReadBuffer(GL_COLOR_ATTACHMENT0));
	}

	// TODO: return something more error-y
	GLenum status = CHECK(tfx_glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		assert(0);
		return false;
	}

	return true;
}

tfx_canvas tfx_canvas_attachments_new(bool claim_attachments, int count, tfx_texture *attachments) {
	assert(did_you_call_tfx_reset);

	tfx_canvas c;
	memset(&c, 0, sizeof(tfx_canvas));

	c.allocated = count;
	c.width = attachments[0].width;
	c.height = attachments[0].height;
	c.current_width = c.width;
	c.current_height = c.height;
	c.own_attachments = claim_attachments;
	c.cube = (attachments[0].flags & TFX_TEXTURE_CUBE) == TFX_TEXTURE_CUBE;
	bool msaa = (attachments[0].flags & TFX_TEXTURE_MSAA_X2) == TFX_TEXTURE_MSAA_X2;
	if ((attachments[0].flags & TFX_TEXTURE_MSAA_X4) == TFX_TEXTURE_MSAA_X4) {
		msaa = true;
	}
	c.msaa = msaa;
	assert(!(c.msaa && c.cube));
	assert(!(c.msaa && attachments[0].depth > 1));

	bool msaa_sample = (attachments[0].flags & TFX_TEXTURE_MSAA_SAMPLE) == TFX_TEXTURE_MSAA_SAMPLE;
	for (int i = 0; i < count; i++) {
		assert(attachments[i].gl_count == 1 || (msaa && msaa_sample));
		assert(attachments[i].depth == attachments[0].depth);
		assert((attachments[i].flags & TFX_TEXTURE_CPU_WRITABLE) != TFX_TEXTURE_CPU_WRITABLE);
		c.attachments[i] = attachments[i];
	}

	// and now the fbo.
	CHECK(tfx_glGenFramebuffers(c.msaa ? 2 : 1, c.gl_fbo));
	CHECK(tfx_glBindFramebuffer(GL_FRAMEBUFFER, c.gl_fbo[0]));

	if (!canvas_reconfigure(&c, false)) {
		tfx_canvas_free(&c);
		return c;
	}

	if (c.gl_fbo[1]) {
		CHECK(tfx_glBindFramebuffer(GL_FRAMEBUFFER, c.gl_fbo[1]));

		if (msaa_sample) {
			if (!canvas_reconfigure(&c, true)) {
				tfx_canvas_free(&c);
			}
			return c;
		}

		int offset = 0;
		// sanity checking was already done by canvas_reconfigure, no need here.
		for (unsigned i = 0; i < c.allocated; i++) {
			GLenum attach = GL_COLOR_ATTACHMENT0 + offset;
			if (c.attachments[i].is_depth && c.attachments[i].is_stencil) {
				attach = GL_DEPTH_STENCIL_ATTACHMENT;
			}
			else if (c.attachments[i].is_stencil) {
				attach = GL_STENCIL_ATTACHMENT;
			}
			else if (c.attachments[i].is_depth) {
				attach = GL_DEPTH_ATTACHMENT;
			}
			else {
				offset += 1;
			}
			CHECK(tfx_glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER, c.attachments[i].gl_msaa_id));
		}
	}

	return c;
}

void tfx_canvas_free(tfx_canvas *c) {
	if (!c->allocated) {
		return;
	}
	CHECK(tfx_glDeleteFramebuffers(c->msaa ? 2 : 1, c->gl_fbo));
	if (!c->own_attachments) {
		return;
	}
	for (unsigned i = 0; i < c->allocated; i++) {
		tfx_texture *attach = &c->attachments[i];
		tfx_texture_free(attach);
	}
	c->allocated = 0;
	c->gl_fbo[0] = 0;
	c->gl_fbo[1] = 0;
}

tfx_canvas tfx_canvas_new(uint16_t w, uint16_t h, tfx_format format, uint16_t flags) {
	assert(did_you_call_tfx_reset);

	tfx_texture attachments[2];
	int n = 0;

	if ((flags & TFX_TEXTURE_EXTERNAL) == TFX_TEXTURE_EXTERNAL) {
		tfx_canvas c;
		memset(&c, 0, sizeof(tfx_canvas));
		c.allocated = 0;
		c.own_attachments = false;
		c.width = w;
		c.height = h;
		c.current_height = h;
		c.current_width = w;
		return c;
	}

	bool has_color = false;
	bool has_depth = false;

	tfx_format color_fmt = TFX_FORMAT_RGBA8;
	tfx_format depth_fmt = TFX_FORMAT_D16;

	switch (format) {
		// just data
		case TFX_FORMAT_R16F:
		case TFX_FORMAT_R32F:
		case TFX_FORMAT_RG16F:
		case TFX_FORMAT_RG32F:
		// color formats
		case TFX_FORMAT_RGB565:
		case TFX_FORMAT_RGBA8:
		case TFX_FORMAT_RGB10A2:
		case TFX_FORMAT_RG11B10F:
		case TFX_FORMAT_RGBA16F: {
			has_color = true;
			color_fmt = format;
			break;
		}
		// color + depth
		case TFX_FORMAT_RGB565_D16: {
			has_depth = true;
			has_color = true;
			color_fmt = TFX_FORMAT_RGB565;
			depth_fmt = TFX_FORMAT_D16;
			break;
		}
		case TFX_FORMAT_RGBA8_D16: {
			has_depth = true;
			has_color = true;
			color_fmt = TFX_FORMAT_RGBA8;
			depth_fmt = TFX_FORMAT_D16;
			break;
		}
		case TFX_FORMAT_RGBA8_D24: {
			has_depth = true;
			has_color = true;
			color_fmt = TFX_FORMAT_RGBA8;
			depth_fmt = TFX_FORMAT_D24;
			break;
		}
		// depth only
		case TFX_FORMAT_D16:
		case TFX_FORMAT_D24: {
			has_depth = true;
			depth_fmt = format;
			break;
		}
		case TFX_FORMAT_D32: {
			has_depth = true;
			depth_fmt = format;
			break;
		}
		default: assert(0);
	}

	if (has_color) {
		attachments[n++] = tfx_texture_new(w, h, 1, NULL, color_fmt, flags);
	}

	if (has_depth) {
		attachments[n++] = tfx_texture_new(w, h, 1, NULL, depth_fmt, flags);
	}

	return tfx_canvas_attachments_new(true, n, attachments);
}

static size_t uniform_size_for(tfx_uniform_type type) {
	switch (type) {
		case TFX_UNIFORM_FLOAT: return sizeof(float);
		case TFX_UNIFORM_INT: return sizeof(uint32_t);
		case TFX_UNIFORM_VEC2: return sizeof(float)*2;
		case TFX_UNIFORM_VEC3: return sizeof(float)*3;
		case TFX_UNIFORM_VEC4: return sizeof(float)*4;
		case TFX_UNIFORM_MAT2: return sizeof(float)*4;
		case TFX_UNIFORM_MAT3: return sizeof(float)*9;
		case TFX_UNIFORM_MAT4: return sizeof(float)*16;
		default: return 0;
	}
	return 0;
}

tfx_uniform tfx_uniform_new(const char *name, tfx_uniform_type type, int count) {
	tfx_uniform u;
	memset(&u, 0, sizeof(tfx_uniform));

	u.name  = name;
	u.type  = type;
	u.count = count;
	u.last_count = count;
	u.size  = count * uniform_size_for(type);

	return u;
}

void tfx_set_uniform(tfx_uniform *uniform, const float *data, const int count) {
	size_t size = uniform->size;
	uniform->last_count = uniform->count;
	if (count >= 0) {
		size = count * uniform_size_for(uniform->type);
		uniform->last_count = count;
	}

	uniform->data = g_back.ub_cursor;
	uint32_t offset = (g_back.ub_cursor + size - g_back.uniform_buffer);
	assert(offset < TFX_UNIFORM_BUFFER_SIZE);
	memcpy(uniform->fdata, data, size);
	g_back.ub_cursor += size;

	sb_push(g_back.uniforms, *uniform);
}

void tfx_set_uniform_int(tfx_uniform *uniform, const int *data, const int count) {
	size_t size = uniform->size;
	uniform->last_count = uniform->count;
	if (count >= 0) {
		size = count * uniform_size_for(uniform->type);
		uniform->last_count = count;
	}

	uniform->data = g_back.ub_cursor;
	uint32_t offset = (g_back.ub_cursor + size - g_back.uniform_buffer);
	assert(offset < TFX_UNIFORM_BUFFER_SIZE);
	memcpy(uniform->idata, data, size);
	g_back.ub_cursor += size;

	sb_push(g_back.uniforms, *uniform);
}

void tfx_view_set_flags(uint8_t id, tfx_view_flags flags) {
	tfx_view *view = &g_back.views[id];
#define FLAG(flags, mask) ((flags & mask) == mask)
	if (FLAG(flags, TFX_VIEW_INVALIDATE)) {
		view->flags |= TFXI_VIEW_INVALIDATE;
	}
	if (FLAG(flags, TFX_VIEW_FLUSH)) {
		view->flags |= TFXI_VIEW_FLUSH;
	}
	// NYI
	if (FLAG(flags, TFX_VIEW_SORT_SEQUENTIAL)) {
		assert(0);
		view->flags |= TFXI_VIEW_SORT_SEQUENTIAL;
	}
#undef FLAG
}

void tfx_view_set_transform(uint8_t id, float *_view, float *proj_l, float *proj_r) {
	// TODO: reserve tfx_world_to_view, tfx_view_to_screen uniforms
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);
	memcpy(view->view, _view, sizeof(float)*16);
	memcpy(view->proj_left, proj_l, sizeof(float)*16);
	memcpy(view->proj_right, proj_r, sizeof(float)*16);
}

void tfx_view_set_name(uint8_t id, const char *name) {
	tfx_view *view = &g_back.views[id];
	view->name = name;
}

void tfx_view_set_canvas(uint8_t id, tfx_canvas *canvas, int layer) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);
	view->has_canvas = true;
	view->canvas = *canvas;
	view->canvas_layer = layer;
}

void tfx_view_set_clear_color(uint8_t id, unsigned color) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);
	view->flags |= TFXI_VIEW_CLEAR_COLOR;
	view->clear_color = color;
}

void tfx_view_set_clear_depth(uint8_t id, float depth) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);
	view->flags |= TFXI_VIEW_CLEAR_DEPTH;
	view->clear_depth = depth;
}

void tfx_view_set_depth_test(uint8_t id, tfx_depth_test mode) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);

	view->flags &= ~TFXI_VIEW_DEPTH_TEST_MASK;
	switch (mode) {
		case TFX_DEPTH_TEST_NONE: break; /* already cleared */
		case TFX_DEPTH_TEST_LT: {
			view->flags |= TFXI_VIEW_DEPTH_TEST_LT;
			break;
		}
		case TFX_DEPTH_TEST_GT: {
			view->flags |= TFXI_VIEW_DEPTH_TEST_GT;
			break;
		}
		case TFX_DEPTH_TEST_EQ: {
			view->flags |= TFXI_VIEW_DEPTH_TEST_EQ;
			break;
		}
		default: assert(0); break;
	}
}

static tfx_canvas *get_canvas(tfx_view *view) {
	assert(view != NULL);
	if (view->has_canvas) {
		return &view->canvas;
	}
	return &g_backbuffer;
}

tfx_canvas *tfx_view_get_canvas(uint8_t id) {
	return get_canvas(&g_back.views[id]);
}

uint16_t tfx_view_get_width(uint8_t id) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);

	if (view->has_canvas) {
		return view->canvas.width;
	}

	return g_backbuffer.width;
}

uint16_t tfx_view_get_height(uint8_t id) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);

	if (view->has_canvas) {
		return view->canvas.height;
	}

	return g_backbuffer.height;
}

void tfx_view_get_dimensions(uint8_t id, uint16_t *w, uint16_t *h) {
	if (w) {
		*w = tfx_view_get_width(id);
	}
	if (h) {
		*h = tfx_view_get_height(id);
	}
}

void tfx_view_set_viewports(uint8_t id, int count, uint16_t **viewports) {
	// as of 2019-01-04, every GPU with GL_ARB_viewport_array supports 16.
	assert(count <= 16);
	if (count > 1) {
		assert(tfx_glViewportIndexedf);
	}

	tfx_view *view = &g_back.views[id];
	view->viewport_count = count;
	for (int i = 0; i < count; i++) {
		view->viewports[i].x = viewports[i][0];
		view->viewports[i].y = viewports[i][1];
		view->viewports[i].w = viewports[i][2];
		view->viewports[i].h = viewports[i][3];
	}
}

void tfx_view_set_instance_mul(uint8_t id, unsigned factor) {
	if (!g_caps.instancing) {
		TFX_WARN("%s", "Instancing is not supported, instance mul will be ignored!");
	}
	tfx_view *view = &g_back.views[id];
	view->instance_mul = factor;
}

void tfx_view_set_scissor(uint8_t id, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	tfx_view *view = &g_back.views[id];
	view->flags |= TFXI_VIEW_SCISSOR;

	tfx_rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	view->scissor_rect = rect;
}

static tfx_draw g_tmp_draw;

static void reset() {
	memset(&g_tmp_draw, 0, sizeof(tfx_draw));
}

void tfx_set_callback(tfx_draw_callback cb) {
	g_tmp_draw.callback = cb;
}

void tfx_set_scissor(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	g_tmp_draw.use_scissor = true;
	g_tmp_draw.scissor_rect.x = x;
	g_tmp_draw.scissor_rect.y = y;
	g_tmp_draw.scissor_rect.w = w;
	g_tmp_draw.scissor_rect.h = h;
}

void tfx_set_texture(tfx_uniform *uniform, tfx_texture *tex, uint8_t slot) {
	assert(slot <= 8);
	assert(uniform != NULL);
	assert(uniform->count == 1);

	uniform->data = g_back.ub_cursor;
	uniform->idata[0] = slot;
	g_back.ub_cursor += uniform->size;

	sb_push(g_back.uniforms, *uniform);

	assert(tex->gl_ids[tex->gl_idx] > 0);
	g_tmp_draw.textures[slot] = *tex;
}

tfx_texture tfx_get_texture(tfx_canvas *canvas, uint8_t index) {
	tfx_texture tex;
	memset(&tex, 0, sizeof(tfx_texture));

	assert(index < canvas->allocated);

	memcpy(&tex, &canvas->attachments[index], sizeof(tfx_texture));
	tex.gl_count = 1;
	tex.gl_ids[0] = tex.gl_ids[tex.gl_idx];
	tex.gl_idx = 0;

	// no cpu writes for textures from canvases, doesn't make sense.
	tex.flags &= ~TFX_TEXTURE_CPU_WRITABLE;

	return tex;
}

void tfx_set_state(uint64_t flags) {
	g_tmp_draw.flags = flags;
}

void tfx_set_buffer(tfx_buffer *buf, uint8_t slot, bool write) {
	assert(slot < 8);
	assert(buf != NULL);
	g_tmp_draw.ssbos[slot] = *buf;
	g_tmp_draw.ssbo_write[slot] = write;
}

void tfx_set_image(tfx_uniform *uniform, tfx_texture *tex, uint8_t slot, uint8_t mip, bool write) {
	assert(slot < 8);
	assert(tex != NULL);
	tfx_set_texture(uniform, tex, slot);
	g_tmp_draw.textures_mip[slot] = mip;
	g_tmp_draw.textures_write[slot] = write;
}

// TODO: make this work for index buffers
void tfx_set_transient_buffer(tfx_transient_buffer tb) {
	assert(tb.has_format);
	g_tmp_draw.vbo = g_transient_buffer.buffers[0];
	g_tmp_draw.use_vbo = true;
	g_tmp_draw.use_tvb = true;
	g_tmp_draw.tvb_fmt = tb.format;
	g_tmp_draw.offset = tb.offset;
	g_tmp_draw.indices = tb.num;
}

void tfx_set_vertices(tfx_buffer *vbo, int count) {
	assert(vbo != NULL);
	assert(vbo->has_format);

	g_tmp_draw.vbo = *vbo;
	g_tmp_draw.use_vbo = true;
	if (!g_tmp_draw.use_ibo) {
		g_tmp_draw.indices = count;
	}
}

void tfx_set_indices(tfx_buffer *ibo, int count, int offset) {
	g_tmp_draw.ibo = *ibo;
	g_tmp_draw.use_ibo = true;
	g_tmp_draw.offset = offset;
	g_tmp_draw.indices = count;
}

static void push_uniforms(tfx_program program, tfx_draw *add_state) {
	tfx_set **found = tfx_set_new();

	int n = sb_count(g_back.uniforms);
	for (int i = n-1; i >= 0; i--) {
		tfx_uniform uniform = g_back.uniforms[i];

		tfx_shadermap *val = tfx_proglookup(g_back.uniform_map, program);
		if (!val) {
			val = tfx_progset(g_back.uniform_map, program);
		}
#ifdef TFX_DEBUG
		assert(val);
		assert(val->value);
#endif
		tfx_locmap **locmap = val->value;
		tfx_locmap *locval = tfx_loclookup(locmap, uniform.name);

		if (!locval) {
			GLint loc = CHECK(tfx_glGetUniformLocation(program, uniform.name));
			if (loc >= 0) {
				locval = tfx_locset(locmap, uniform.name, loc);
			}
			else {
				continue;
			}
		}

		// only record the last update for a given uniform
		if (!tfx_slookup(found, uniform.name)) {
			tfx_uniform found_uniform = uniform;
			found_uniform.data = g_back.ub_cursor;
			assert((g_back.ub_cursor + uniform.size - g_back.uniform_buffer) < TFX_UNIFORM_BUFFER_SIZE);
			memcpy(found_uniform.data, uniform.data, uniform.size);
			g_back.ub_cursor += uniform.size;

			tfx_sset(found, uniform.name);
			sb_push(add_state->uniforms, found_uniform);
		}
	}

	tfx_set_delete(found);
}

void tfx_dispatch(uint8_t id, tfx_program program, uint32_t x, uint32_t y, uint32_t z) {
	tfx_view *view = &g_back.views[id];
	g_tmp_draw.program = program;
	assert(program != 0);
	assert(view != NULL);
	assert((x*y*z) > 0);

	tfx_draw add_state;
	memcpy(&add_state, &g_tmp_draw, sizeof(tfx_draw));
	add_state.threads_x = x;
	add_state.threads_y = y;
	add_state.threads_z = z;

	push_uniforms(program, &add_state);
	sb_push(view->jobs, add_state);

	reset();
}

void tfx_submit(uint8_t id, tfx_program program, bool retain) {
	tfx_view *view = &g_back.views[id];
	g_tmp_draw.program = program;
	assert(program != 0);
	assert(view != NULL);

	tfx_draw add_state;
	memcpy(&add_state, &g_tmp_draw, sizeof(tfx_draw));
	push_uniforms(program, &add_state);
	sb_push(view->draws, add_state);

	if (!retain) {
		reset();
	}
}

void tfx_submit_ordered(uint8_t id, tfx_program program, uint32_t depth, bool retain) {
	g_tmp_draw.depth = depth;
	tfx_submit(id, program, retain);
}

void tfx_touch(uint8_t id) {
	tfx_view *view = &g_back.views[id];
	assert(view != NULL);

	tfx_draw_callback cb = g_tmp_draw.callback;
	uint64_t flags = g_tmp_draw.flags;
	reset();
	g_tmp_draw.callback = cb;
	if (g_tmp_draw.callback) {
		g_tmp_draw.flags = flags;
	}
	sb_push(view->draws, g_tmp_draw);
	g_tmp_draw.callback = NULL;
	g_tmp_draw.flags = 0;
}

void tfx_blit(uint8_t dst, uint8_t src, uint16_t x, uint16_t y, uint16_t w, uint16_t h, int mip) {
	tfx_rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	tfx_blit_op blit;
	blit.source = get_canvas(&g_back.views[src]);
	blit.source_mip = mip;
	blit.rect = rect;
	blit.mask = 0;

	tfx_view *view = &g_back.views[dst];
	tfx_canvas *canvas = get_canvas(view);

	// blit to self doesn't make sense, and msaa resolve is automatic.
	assert(blit.source != canvas);

	for (unsigned i = 0; i < canvas->allocated; i++) {
		tfx_texture *attach = &canvas->attachments[i];
		if (attach->is_stencil) {
			blit.mask |= GL_STENCIL_BUFFER_BIT;
		}
		if (attach->is_depth) {
			blit.mask |= GL_DEPTH_BUFFER_BIT;
		}
		// there aren't any combined color+depth or stencil
		if (!attach->is_depth && !attach->is_stencil) {
			blit.mask |= GL_COLOR_BUFFER_BIT;
		}
	}

	sb_push(view->blits, blit);
}

static void release_compiler() {
	if (!g_shaderc_allocated) {
		return;
	}

	int release_shader_c = 0;
	CHECK(tfx_glGetIntegerv(GL_SHADER_COMPILER, &release_shader_c));

	if (release_shader_c) {
		CHECK(tfx_glReleaseShaderCompiler());
	}

	g_shaderc_allocated = false;
}

static void update_uniforms(tfx_draw *draw) {
	int nu = sb_count(draw->uniforms);
	for (int j = 0; j < nu; j++) {
		tfx_uniform uniform = draw->uniforms[j];

		tfx_shadermap *val = tfx_proglookup(g_back.uniform_map, draw->program);
		tfx_locmap **locmap = val->value;
		tfx_locmap *locval = tfx_loclookup(locmap, uniform.name);
#ifdef TFX_DEBUG
		assert(locval);
#endif

		GLint loc = locval->value;
		if (loc < 0) {
			continue;
		}
		switch (uniform.type) {
			case TFX_UNIFORM_INT:   CHECK(tfx_glUniform1iv(loc, uniform.last_count, uniform.idata)); break;
			case TFX_UNIFORM_FLOAT: CHECK(tfx_glUniform1fv(loc, uniform.last_count, uniform.fdata)); break;
			case TFX_UNIFORM_VEC2:  CHECK(tfx_glUniform2fv(loc, uniform.last_count, uniform.fdata)); break;
			case TFX_UNIFORM_VEC3:  CHECK(tfx_glUniform3fv(loc, uniform.last_count, uniform.fdata)); break;
			case TFX_UNIFORM_VEC4:  CHECK(tfx_glUniform4fv(loc, uniform.last_count, uniform.fdata)); break;
			case TFX_UNIFORM_MAT2:  CHECK(tfx_glUniformMatrix2fv(loc, uniform.last_count, 0, uniform.fdata)); break;
			case TFX_UNIFORM_MAT3:  CHECK(tfx_glUniformMatrix3fv(loc, uniform.last_count, 0, uniform.fdata)); break;
			case TFX_UNIFORM_MAT4:  CHECK(tfx_glUniformMatrix4fv(loc, uniform.last_count, 0, uniform.fdata)); break;
			default: assert(0); break;
		}
	}
}

// 8x8 font based on the ibm (?) vga bios font, based on the haxe vga text renderer example.
// TODO: change this to an 8x16 or so font, it'll look much better.
static const char vga_font[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x7E,0x81,0xA5,0x81,0xBD,0x99,0x81,0x7E,
	0x7E,0xFF,0x00,0xFF,0xC3,0xE7,0xFF,0x7E,
	0x6C,0xFE,0xFE,0xFE,0x7C,0x38,0x10,0x00,
	0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10,0x00,
	0x38,0x7C,0x38,0xFE,0xFE,0x92,0x10,0x7C,
	0x00,0x10,0x38,0x7C,0xFE,0x7C,0x38,0x7C,
	0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x00,
	0xFF,0xFF,0xE7,0xC3,0xC3,0xE7,0xFF,0xFF,
	0x00,0x3C,0x66,0x42,0x42,0x66,0x3C,0x00,
	0xFF,0xC3,0x99,0xBD,0xBD,0x99,0xC3,0xFF,
	0x0F,0x07,0x0F,0x7D,0xCC,0xCC,0xCC,0x78,
	0x3C,0x66,0x66,0x66,0x3C,0x18,0x7E,0x18,
	0x3F,0x33,0x3F,0x30,0x30,0x70,0xF0,0xE0,
	0x7F,0x63,0x7F,0x63,0x63,0x67,0xE6,0xC0,
	0x99,0x5A,0x3C,0xE7,0xE7,0x3C,0x5A,0x99,
	0x80,0xE0,0xF8,0xFE,0xF8,0xE0,0x80,0x00,
	0x02,0x0E,0x3E,0xFE,0x3E,0x0E,0x02,0x00,
	0x18,0x3C,0x7E,0x18,0x18,0x7E,0x3C,0x18,
	0x66,0x66,0x66,0x66,0x66,0x00,0x66,0x00,
	0x7F,0x00,0x00,0x7B,0x1B,0x1B,0x1B,0x00,
	0x3E,0x63,0x38,0x6C,0x6C,0x38,0x86,0xFC,
	0x00,0x00,0x00,0x00,0x7E,0x7E,0x7E,0x00,
	0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0xFF,
	0x18,0x3C,0x7E,0x18,0x18,0x18,0x18,0x00,
	0x18,0x18,0x18,0x18,0x7E,0x3C,0x18,0x00,
	0x00,0x18,0x0C,0xFE,0x0C,0x18,0x00,0x00,
	0x00,0x30,0x60,0xFE,0x60,0x30,0x00,0x00,
	0x00,0x00,0xC0,0xC0,0xC0,0xFE,0x00,0x00,
	0x00,0x24,0x66,0xFF,0x66,0x24,0x00,0x00,
	0x00,0x18,0x3C,0x7E,0xFF,0xFF,0x00,0x00,
	0x00,0xFF,0xFF,0x7E,0x3C,0x18,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00,
	0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,
	0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,
	0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00,
	0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00,
	0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00,
	0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00,
	0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00,
	0x60,0x30,0x18,0x18,0x18,0x30,0x60,0x00,
	0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,
	0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30,
	0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,
	0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,
	0x7C,0xCE,0xDE,0xF6,0xE6,0xC6,0x7C,0x00,
	0x30,0x70,0x30,0x30,0x30,0x30,0xFC,0x00,
	0x78,0xCC,0x0C,0x38,0x60,0xCC,0xFC,0x00,
	0x78,0xCC,0x0C,0x38,0x0C,0xCC,0x78,0x00,
	0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00,
	0xFC,0xC0,0xF8,0x0C,0x0C,0xCC,0x78,0x00,
	0x38,0x60,0xC0,0xF8,0xCC,0xCC,0x78,0x00,
	0xFC,0xCC,0x0C,0x18,0x30,0x30,0x30,0x00,
	0x78,0xCC,0xCC,0x78,0xCC,0xCC,0x78,0x00,
	0x78,0xCC,0xCC,0x7C,0x0C,0x18,0x70,0x00,
	0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00,
	0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30,
	0x18,0x30,0x60,0xC0,0x60,0x30,0x18,0x00,
	0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,
	0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00,
	0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00,
	0x7C,0xC6,0xDE,0xDE,0xDC,0xC0,0x7C,0x00,
	0x30,0x78,0xCC,0xCC,0xFC,0xCC,0xCC,0x00,
	0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00,
	0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
	0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00,
	0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00,
	0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00,
	0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00,
	0xCC,0xCC,0xCC,0xFC,0xCC,0xCC,0xCC,0x00,
	0x78,0x30,0x30,0x30,0x30,0x30,0x78,0x00,
	0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,
	0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00,
	0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00,
	0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
	0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00,
	0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00,
	0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,
	0x7C,0xC6,0xC6,0xC6,0xD6,0x7C,0x0E,0x00,
	0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00,
	0x7C,0xC6,0xE0,0x78,0x0E,0xC6,0x7C,0x00,
	0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00,
	0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00,
	0xC6,0xC6,0xC6,0xC6,0xD6,0xFE,0x6C,0x00,
	0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00,
	0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00,
	0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00,
	0x78,0x60,0x60,0x60,0x60,0x60,0x78,0x00,
	0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,
	0x78,0x18,0x18,0x18,0x18,0x18,0x78,0x00,
	0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
	0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00,
	0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00,
	0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00,
	0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00,
	0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00,
	0x38,0x6C,0x64,0xF0,0x60,0x60,0xF0,0x00,
	0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8,
	0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00,
	0x30,0x00,0x70,0x30,0x30,0x30,0x78,0x00,
	0x0C,0x00,0x1C,0x0C,0x0C,0xCC,0xCC,0x78,
	0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00,
	0x70,0x30,0x30,0x30,0x30,0x30,0x78,0x00,
	0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xD6,0x00,
	0x00,0x00,0xB8,0xCC,0xCC,0xCC,0xCC,0x00,
	0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00,
	0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0,
	0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E,
	0x00,0x00,0xDC,0x76,0x62,0x60,0xF0,0x00,
	0x00,0x00,0x7C,0xC0,0x70,0x1C,0xF8,0x00,
	0x10,0x30,0xFC,0x30,0x30,0x34,0x18,0x00,
	0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00,
	0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00,
	0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00,
	0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00,
	0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8,
	0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00,
	0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00,
	0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,
	0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00,
	0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00
};

static void render_letter(
	const int pitch,
	const int charCode,
	const int baseRow,
	const int baseCol,
	const uint32_t fg,
	const uint32_t bg,
	uint32_t *pixels) {
	// Calculate character index in font character array
	// Shifting by 3 to the left is equal to multiplying by 2^3
	unsigned baseIndex = charCode << 3;
	// Iterate over character raster rows
	for (unsigned curRow = 0; curRow < 8; curRow++) {
		// Get character raster row bits
		const unsigned rowBits = vga_font[baseIndex + curRow];
		const unsigned y = baseRow + curRow + 2;
		// Iterate over character raster row bits (columns)
		for (unsigned curCol = 0; curCol < 8; curCol++) {
			// Calculate position of pixel on image
			unsigned x = baseCol + curCol + 2;
			// Calculate pixel index in image data array
			unsigned index = y * pitch + x;
			// assert(x < g_debug_overlay.width);
			// assert(y < g_debug_overlay.height);
			// Determine pixel color based on current bit value
			uint32_t fill = (((rowBits << curCol) & 0x80) == 0x80) ? fg : bg;
			pixels[index] = fill;
		}
	}
}

static tfx_transient_buffer screen_triangle() {
	tfx_vertex_format fmt = tfx_vertex_format_start();
	tfx_vertex_format_add(&fmt, 0, 3, false, TFX_TYPE_FLOAT);
	tfx_vertex_format_end(&fmt);

	tfx_transient_buffer tb = tfx_transient_buffer_new(&fmt, 3);
	float *fdata = (float*)tb.data;
	const float depth = 0.0f;
	fdata[0] = -1.0f;
	fdata[1] = 3.0f;
	fdata[2] = depth;
	fdata[3] = -1.0f;
	fdata[4] = -1.0f;
	fdata[5] = depth;
	fdata[6] = 3.0f;
	fdata[7] = -1.0f;
	fdata[8] = depth;
	return tb;
}

static uint32_t g_debug_default_palette[256] = {
	0xFF000000, 0xFFAA0000, 0xFF00AA00, 0xFFAAAA00, 0xFF0000AA, 0xFFAA00AA, 0xFF0055AA, 0xFFAAAAAA,
	0xFF555555, 0xFFFF5555, 0xFF55FF55, 0xFFFFFF55, 0xFF5555FF, 0xFFFF55FF, 0xFF55FFFF, 0xFFFFFFFF,
	0xFF000000, 0xFF141414, 0xFF202020, 0xFF2C2C2C, 0xFF383838, 0xFF454545, 0xFF515151, 0xFF616161,
	0xFF717171, 0xFF828282, 0xFF929292, 0xFFA2A2A2, 0xFFB6B6B6, 0xFFCBCBCB, 0xFFE3E3E3, 0xFFFFFFFF,
	0xFFFF0000, 0xFFFF0041, 0xFFFF007D, 0xFFFF00BE, 0xFFFF00FF, 0xFFBE00FF, 0xFF7D00FF, 0xFF4100FF,
	0xFF0000FF, 0xFF0041FF, 0xFF007DFF, 0xFF00BEFF, 0xFF00FFFF, 0xFF00FFBE, 0xFF00FF7D, 0xFF00FF41,
	0xFF00FF00, 0xFF41FF00, 0xFF7DFF00, 0xFFBEFF00, 0xFFFFFF00, 0xFFFFBE00, 0xFFFF7D00, 0xFFFF4100,
	0xFFFF7D7D, 0xFFFF7D9E, 0xFFFF7DBE, 0xFFFF7DDF, 0xFFFF7DFF, 0xFFDF7DFF, 0xFFBE7DFF, 0xFF9E7DFF,
	0xFF7D7DFF, 0xFF7D9EFF, 0xFF7DBEFF, 0xFF7DDFFF, 0xFF7DFFFF, 0xFF7DFFDF, 0xFF7DFFBE, 0xFF7DFF9E,
	0xFF7DFF7D, 0xFF9EFF7D, 0xFFBEFF7D, 0xFFDFFF7D, 0xFFFFFF7D, 0xFFFFDF7D, 0xFFFFBE7D, 0xFFFF9E7D,
	0xFFFFB6B6, 0xFFFFB6C7, 0xFFFFB6DB, 0xFFFFB6EB, 0xFFFFB6FF, 0xFFEBB6FF, 0xFFDBB6FF, 0xFFC7B6FF,
	0xFFB6B6FF, 0xFFB6C7FF, 0xFFB6DBFF, 0xFFB6EBFF, 0xFFB6FFFF, 0xFFB6FFEB, 0xFFB6FFDB, 0xFFB6FFC7,
	0xFFB6FFB6, 0xFFC7FFB6, 0xFFDBFFB6, 0xFFEBFFB6, 0xFFFFFFB6, 0xFFFFEBB6, 0xFFFFDBB6, 0xFFFFC7B6,
	0xFF710000, 0xFF71001C, 0xFF710038, 0xFF710055, 0xFF710071, 0xFF550071, 0xFF380071, 0xFF1C0071,
	0xFF000071, 0xFF001C71, 0xFF003871, 0xFF005571, 0xFF007171, 0xFF007155, 0xFF007138, 0xFF00711C,
	0xFF007100, 0xFF1C7100, 0xFF387100, 0xFF557100, 0xFF717100, 0xFF715500, 0xFF713800, 0xFF711C00,
	0xFF713838, 0xFF713845, 0xFF713855, 0xFF713861, 0xFF713871, 0xFF613871, 0xFF553871, 0xFF453871,
	0xFF383871, 0xFF384571, 0xFF385571, 0xFF386171, 0xFF387171, 0xFF387161, 0xFF387155, 0xFF387145,
	0xFF387138, 0xFF457138, 0xFF557138, 0xFF617138, 0xFF717138, 0xFF716138, 0xFF715538, 0xFF714538,
	0xFF715151, 0xFF715159, 0xFF715161, 0xFF715169, 0xFF715171, 0xFF695171, 0xFF615171, 0xFF595171,
	0xFF515171, 0xFF515971, 0xFF516171, 0xFF516971, 0xFF517171, 0xFF517169, 0xFF517161, 0xFF517159,
	0xFF517151, 0xFF597151, 0xFF617151, 0xFF697151, 0xFF717151, 0xFF716951, 0xFF716151, 0xFF715951,
	0xFF410000, 0xFF410010, 0xFF410020, 0xFF410030, 0xFF410041, 0xFF300041, 0xFF200041, 0xFF100041,
	0xFF000041, 0xFF001041, 0xFF002041, 0xFF003041, 0xFF004141, 0xFF004130, 0xFF004120, 0xFF004110,
	0xFF004100, 0xFF104100, 0xFF204100, 0xFF304100, 0xFF414100, 0xFF413000, 0xFF412000, 0xFF411000,
	0xFF412020, 0xFF412028, 0xFF412030, 0xFF412038, 0xFF412041, 0xFF382041, 0xFF302041, 0xFF282041,
	0xFF202041, 0xFF202841, 0xFF203041, 0xFF203841, 0xFF204141, 0xFF204138, 0xFF204130, 0xFF204128,
	0xFF204120, 0xFF284120, 0xFF304120, 0xFF384120, 0xFF414120, 0xFF413820, 0xFF413020, 0xFF412820,
	0xFF412C2C, 0xFF412C30, 0xFF412C34, 0xFF412C3C, 0xFF412C41, 0xFF3C2C41, 0xFF342C41, 0xFF302C41,
	0xFF2C2C41, 0xFF2C3041, 0xFF2C3441, 0xFF2C3C41, 0xFF2C4141, 0xFF2C413C, 0xFF2C4134, 0xFF2C4130,
	0xFF2C412C, 0xFF30412C, 0xFF34412C, 0xFF3C412C, 0xFF41412C, 0xFF413C2C, 0xFF41342C, 0xFF41302C,
	0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00000000
};

static uint32_t *g_debug_palette = g_debug_default_palette;

void tfx_debug_set_palette(const uint32_t *palette) {
	g_debug_palette = (uint32_t *)palette;
	if (!g_debug_palette) {
		g_debug_palette = g_debug_default_palette;
	}
}

void tfx_debug_blit_rgba(const int x, const int y, const int w, const int h, const uint32_t *pixels) {
	assert(did_you_call_tfx_reset);

	if ((g_flags & TFX_RESET_DEBUG_OVERLAY) != TFX_RESET_DEBUG_OVERLAY) {
		return;
	}

	const unsigned bufw = g_debug_overlay.width;
	const unsigned bufh = g_debug_overlay.height;
	for (unsigned _y = 0; _y < (unsigned)h; _y++) {
		unsigned row_base = (y + _y) * bufw;
		unsigned row_local = _y * w;
		for (unsigned _x = 0; _x < (unsigned)w; _x++) {
			g_debug_data[row_base + _x + x] = pixels[row_local + _x];
		}
	}
}

void tfx_debug_blit_pal(const int x, const int y, const int w, const int h, const uint8_t *pixels) {
	assert(did_you_call_tfx_reset);

	if ((g_flags & TFX_RESET_DEBUG_OVERLAY) != TFX_RESET_DEBUG_OVERLAY) {
		return;
	}

	const unsigned bufw = g_debug_overlay.width;
	const unsigned bufh = g_debug_overlay.height;
	for (unsigned _y = 0; _y < (unsigned)h; _y++) {
		unsigned row_base = (y + _y) * bufw;
		unsigned row_local = _y * w;
		for (unsigned _x = 0; _x < (unsigned)w; _x++) {
			uint8_t idx = pixels[row_local + _x];
			g_debug_data[row_base + _x + x] = g_debug_palette[idx];
		}
	}
}

void tfx_debug_print(const int baserow, const int basecol, const uint16_t bg_fg, const int auto_wrap, const char *str) {
	assert(did_you_call_tfx_reset);

	if ((g_flags & TFX_RESET_DEBUG_OVERLAY) != TFX_RESET_DEBUG_OVERLAY) {
		return;
	}

	const char *head = str;
	const int bufw = g_debug_overlay.width;
	const int bufh = g_debug_overlay.height;
	int row = baserow;
	int col = basecol;
	const size_t bg_idx = bg_fg >> 8;
	const size_t fg_idx = bg_fg & 0xff;
	const uint32_t bg = g_debug_palette[bg_idx];
	const uint32_t fg = g_debug_palette[fg_idx];
	while (*head) {
		char c = *head;
		if (auto_wrap) {
			int skip = c == ' ' && col == 0;
			if (skip) {
				head++;
				continue;
			}
		}
		int newline = c == '\n' || c == '\r';
		int x = (col << 3); // + (col>>1)
		int wrap = x + 8 >= bufw;
		if (!auto_wrap && wrap) {
			head++;
			continue;
		}
		wrap |= newline;
		if (wrap) {
			col = basecol;
			row += 1;
			if (newline) {
				head++;
			}
			continue;
		}
		int y = (row << 3); // + (row >> 1)
		if (y + 8 >= bufh) {
			break;
		}
		render_letter(bufw, *head, y, x, fg, bg, g_debug_data);
		col++;
		head++;
	}
}

tfx_stats tfx_frame() {
	assert(did_you_call_tfx_reset);

	/* This isn't used on RPi, but should free memory on some devices. When
	 * you call tfx_frame, you should be done with your shader compiles for
	 * a good while, since that should only be done during init/loading. */
	release_compiler();

	if (g_caps.debug_output) {
		CHECK(tfx_glEnable(GL_DEBUG_OUTPUT));
	}

	tfx_stats stats;
	memset(&stats, 0, sizeof(tfx_stats));
	stats.timings = last_timings;
	memset(last_timings, 0, sizeof(tfx_timing_info)*VIEW_MAX);

	//CHECK(tfx_glEnable(GL_FRAMEBUFFER_SRGB));

	// I'm not aware of any situation this is available but undesirable,
	// so we use it unconditionally if possible.
	if (g_caps.seamless_cubemap) {
		CHECK(tfx_glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
	}

	GLuint vao;
	if (tfx_glGenVertexArrays && tfx_glBindVertexArray) {
		CHECK(tfx_glGenVertexArrays(1, &vao));
		CHECK(tfx_glBindVertexArray(vao));
	}

	unsigned debug_id = 0;

	// update the debug overlay, make sure to do this before texture update AND buffer update
	if ((g_flags & TFX_RESET_DEBUG_OVERLAY) == TFX_RESET_DEBUG_OVERLAY && g_debug_data != NULL && g_debug_program) {
		push_group(debug_id++, "Update Debug");
		tfx_texture_update(&g_debug_overlay, g_debug_data);

		tfx_view_set_name(254, "Debug");
		tfx_set_texture(&g_debug_texture, &g_debug_overlay, 0);
		tfx_set_transient_buffer(screen_triangle());
		tfx_set_state(TFX_STATE_RGB_WRITE | TFX_STATE_ALPHA_WRITE | TFX_STATE_BLEND_ALPHA | TFX_STATE_CULL_CCW);
		// one before last view id, so you can draw over it if you absolutely must in the final slot.
		tfx_submit(254, g_debug_program, false);
		pop_group();
	}

	push_group(debug_id++, "Update Resources");

	if (g_transient_buffer.offset > 0) {
		CHECK(tfx_glBindBuffer(GL_ARRAY_BUFFER, g_transient_buffer.buffers[0].gl_id));
		if (tfx_glMapBufferRange && tfx_glUnmapBuffer) {
			// this is backed by multiple buffers, so invalidate might be pointless. need to profile.
			void *ptr = tfx_glMapBufferRange(GL_ARRAY_BUFFER, 0, g_transient_buffer.offset, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			if (ptr) {
				memcpy(ptr, g_transient_buffer.data, g_transient_buffer.offset);
				CHECK(tfx_glUnmapBuffer(GL_ARRAY_BUFFER));
			}
		}
		else {
			// orphan the buffer explicitly if mapping isn't available
			CHECK(tfx_glBufferData(GL_ARRAY_BUFFER, TFX_TRANSIENT_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW));
			CHECK(tfx_glBufferSubData(GL_ARRAY_BUFFER, 0, g_transient_buffer.offset, g_transient_buffer.data));
		}
	}

	int nbb = sb_count(g_buffers);
	for (int i = 0; i < nbb; i++) {
		tfx_buffer *buf = &g_buffers[i];
		tfx_buffer_params *params = (tfx_buffer_params*)buf->internal;
		if (params) {
			CHECK(tfx_glBindBuffer(GL_ARRAY_BUFFER, buf->gl_id));
			if (tfx_glMapBufferRange && tfx_glUnmapBuffer) {
				void *ptr = tfx_glMapBufferRange(GL_ARRAY_BUFFER, params->offset, params->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
				if (ptr) {
					memcpy(ptr, params->update_data, params->size);
					CHECK(tfx_glUnmapBuffer(GL_ARRAY_BUFFER));
				}
			}
			else {
				CHECK(tfx_glBufferData(GL_ARRAY_BUFFER, params->size, NULL, GL_DYNAMIC_DRAW));
				CHECK(tfx_glBufferSubData(GL_ARRAY_BUFFER, params->offset, params->size, params->update_data));
			}
			free(params);
			buf->internal = NULL;
		}
	}

	int nt = sb_count(g_textures);
	for (int i = 0; i < nt; i++) {
		tfx_texture *tex = &g_textures[i];
		tfx_texture_params *internal = tex->internal;
		if (internal->update_data != NULL && (tex->flags & TFX_TEXTURE_CPU_WRITABLE) == TFX_TEXTURE_CPU_WRITABLE) {
			assert((tex->flags & TFX_TEXTURE_CUBE) != TFX_TEXTURE_CUBE);
			// spin the buffer id before updating
			tex->gl_idx = (tex->gl_idx + 1) % tex->gl_count;
			tfx_glBindTexture(GL_TEXTURE_2D, tex->gl_ids[tex->gl_idx]);
			if (tfx_glInvalidateTexSubImage && !g_platform_data.use_gles) {
				tfx_glInvalidateTexSubImage(tex->gl_ids[tex->gl_idx], 0, 0, 0, 0, tex->width, tex->height, 1);
			}
			tfx_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->width, tex->height, internal->format, internal->type, internal->update_data);
			internal->update_data = NULL;
		}
	}

	// clear debug pixel data for next frame
	if ((g_flags & TFX_RESET_DEBUG_OVERLAY) == TFX_RESET_DEBUG_OVERLAY && g_debug_data != NULL) {
		size_t pitch = (size_t)g_debug_overlay.width * 4;
		size_t overlay_size = g_debug_overlay.height * pitch;
		memset(g_debug_data, 0, overlay_size);
	}

	pop_group();

	char debug_label[256];

	tfx_canvas *last_canvas = NULL;
	int last_count = 0;
	GLuint last_program = 0;
	GLuint64 last_result = 0;

	// flip active timers every other frame. we get results from previous frame.
	uint32_t next_offset = g_timer_offset;
	next_offset += VIEW_MAX + 1;
	next_offset %= TIMER_COUNT;

	bool in_group = false;

	for (int id = 0; id < VIEW_MAX; id++) {
		tfx_view *view = &g_back.views[id];

		int nd = sb_count(view->draws);
		int cd = sb_count(view->jobs);
		if (nd == 0 && cd == 0) {
			continue;
		}

		if (!in_group || view->name) {
			if (in_group) {
				pop_group();
			}
			if (view->name) {
				snprintf(debug_label, 256, "%s (%d)", view->name, id);
			}
			else {
				snprintf(debug_label, 256, "View %d", id);
			}
			push_group(debug_id++, debug_label);
			in_group = true;
		}

		if (use_timers) {
			int idx = id + next_offset;
			GLuint result_available = 0;
			CHECK(tfx_glGetQueryObjectuiv(g_timers[idx], GL_QUERY_RESULT_AVAILABLE, &result_available));
			if (result_available) {
				GLuint64 result = 0;
				CHECK(tfx_glGetQueryObjectui64v(g_timers[idx], GL_QUERY_RESULT, &result));
				GLuint64 now = result - last_result;
				last_result = result;

				if (stats.num_timings > 0) {
					stats.timings[stats.num_timings-1].time += now;
				}

				if (view->name) {
					stats.timings[stats.num_timings].id = id;
					stats.timings[stats.num_timings].name = view->name;
					stats.num_timings += 1;
				}
			}
			CHECK(tfx_glQueryCounter(g_timers[id + g_timer_offset], GL_TIMESTAMP));
		}

		stats.draws += nd;

		tfx_canvas *canvas = get_canvas(view);

		bool canvas_changed = last_canvas && canvas != last_canvas && last_canvas->gl_fbo[0] != canvas->gl_fbo[0];

		// invalidate before switching canvas, if needed.
		if (canvas_changed) {
			if ((view->flags & TFXI_VIEW_INVALIDATE) == TFXI_VIEW_INVALIDATE && tfx_glInvalidateFramebuffer) {
				int offset = 0;
				GLenum attachments[8];
				for (unsigned i = 0; i < canvas->allocated; i++) {
					tfx_texture *attachment = &canvas->attachments[i];
					if (attachment->is_stencil && attachment->is_depth) {
						attachments[i] = GL_DEPTH_STENCIL_ATTACHMENT;
					}
					else if (attachment->is_stencil) {
						attachments[i] = GL_STENCIL_ATTACHMENT;
					}
					else if (attachment->is_depth) {
						attachments[i] = GL_DEPTH_ATTACHMENT;
					}
					else {
						attachments[i] = GL_COLOR_ATTACHMENT0 + offset;
						offset += 1;
					}
				}
				CHECK(tfx_glInvalidateFramebuffer(GL_FRAMEBUFFER, canvas->allocated, attachments));
			}
		}

		if ((view->flags & TFXI_VIEW_FLUSH) == TFXI_VIEW_FLUSH) {
			CHECK(tfx_glFlush());
		}

		int last_mip = (last_canvas && last_canvas->current_mip) ? last_canvas->current_mip : 0;
		bool reset_mip = canvas_changed && last_mip != 0;
		bool mip_changed = reset_mip || last_mip != view->canvas_layer;
		// reset mipmap level range when done rendering, so sampling works.
		if (reset_mip) {
			int offset = 0;
			for (unsigned i = 0; i < last_canvas->allocated; i++) {
				tfx_texture *attachment = &last_canvas->attachments[i];
				CHECK(tfx_glBindTexture(GL_TEXTURE_2D, attachment->gl_ids[0]));
				CHECK(tfx_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
				CHECK(tfx_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, attachment->mip_count-1));

				GLenum attach = GL_DEPTH_ATTACHMENT;
				if (attachment->is_stencil && attachment->is_depth) {
					attach = GL_DEPTH_STENCIL_ATTACHMENT;
				}
				else if (attachment->is_stencil) {
					attach = GL_STENCIL_ATTACHMENT;
				}
				else if (!attachment->is_depth && !attachment->is_stencil) {
					attach = GL_COLOR_ATTACHMENT0 + offset;
					offset += 1;
				}
				CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, attachment->gl_ids[0], 0));
			}
			last_canvas->current_width = last_canvas->width;
			last_canvas->current_height = last_canvas->height;
			last_canvas->current_mip = 0;
		}

		// resolve msaa if needed
		if (last_canvas
			&& last_canvas->msaa
			&& (last_canvas->attachments[0].flags & TFX_TEXTURE_MSAA_SAMPLE) != TFX_TEXTURE_MSAA_SAMPLE
			&& ((canvas_changed && last_mip == 0) || (mip_changed && last_mip == 0))
		) {
			GLenum mask = 0;
			for (unsigned i = 0; i < last_canvas->allocated; i++) {
				tfx_texture *attach = &last_canvas->attachments[i];
				if (attach->is_stencil) {
					mask |= GL_STENCIL_BUFFER_BIT;
				}
				if (attach->is_depth) {
					mask |= GL_DEPTH_BUFFER_BIT;
				}
				if (!attach->is_depth && !attach->is_stencil) {
					mask |= GL_COLOR_BUFFER_BIT;
				}
			}
			CHECK(tfx_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, last_canvas->gl_fbo[0]));
			CHECK(tfx_glBindFramebuffer(GL_READ_FRAMEBUFFER, last_canvas->gl_fbo[1]));
			CHECK(tfx_glBlitFramebuffer(
				0, 0, last_canvas->width, last_canvas->height, // src
				0, 0, last_canvas->width, last_canvas->height, // dst
				mask, GL_NEAREST
			));
		}
		int nb = sb_count(view->blits);
		stats.blits += nb;

		if (nb > 0) {
			for (int b = 0; b < nb; b++) {
				tfx_blit_op *blit = &view->blits[b];
				tfx_canvas *src = blit->source;
				if (tfx_glCopyImageSubData) {
					int argh = 0;
					if (canvas->attachments[0].is_depth || canvas->attachments[0].is_stencil) {
						argh = 1;
					}
					CHECK(tfx_glCopyImageSubData(
						src->attachments[argh].gl_ids[0], GL_TEXTURE_2D, blit->source_mip,
						blit->rect.x, blit->rect.y, 0,
						canvas->attachments[0].gl_ids[0], GL_TEXTURE_2D, canvas->current_mip,
						blit->rect.x, blit->rect.y, 0,
						blit->rect.w, blit->rect.h, 1
					));
				} else {
					CHECK(tfx_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, canvas->msaa ? canvas->gl_fbo[1] : canvas->gl_fbo[0]));
					CHECK(tfx_glBindFramebuffer(GL_READ_FRAMEBUFFER, src->msaa ? src->gl_fbo[1] : src->gl_fbo[0]));
					if (blit->source_mip != src->current_mip) {
						// TODO: calculate correct dest size, update mip bindings
						assert(0);
						// CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, attachment->gl_ids[0], canvas->current_mip));
					}
					CHECK(tfx_glBlitFramebuffer(
						blit->rect.x, blit->rect.y, blit->rect.w, blit->rect.h, // src
						blit->rect.x, blit->rect.y, blit->rect.w, blit->rect.h, // dst
						blit->mask, GL_NEAREST
					));
				}
			}
		}

		// run compute after blit so compute can rely on msaa being resolved first.
		if (g_caps.compute && cd > 0) {
			for (int i = 0; i < cd; i++) {
				tfx_draw job = view->jobs[i];
				if (job.program != last_program) {
					CHECK(tfx_glUseProgram(job.program));
					last_program = job.program;
				}

				for (int j = 0; j < 8; j++) {
					// make sure writing to image has finished before read
					tfx_texture *tex = &job.textures[j];
					GLuint id = tex->gl_ids[tex->gl_idx];
					if (id != 0) {
						static PFNGLBINDIMAGETEXTUREPROC tfx_glBindImageTexture = NULL;
						if (!tfx_glBindImageTexture) {
							tfx_glBindImageTexture = g_platform_data.gl_get_proc_address("glBindImageTexture");
						}
						if (tex->dirty) {
							CHECK(tfx_glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
							tex->dirty = false;
						}
						bool write = job.textures_write[j];
						if (write) {
							tex->dirty = true;
						}
						tfx_texture_params *internal = (tfx_texture_params*)tex->internal;
						GLenum fmt = internal->internal_format;
						switch (fmt) {
							case GL_DEPTH_COMPONENT16: fmt = GL_R16F; break;
							case GL_DEPTH_COMPONENT24: assert(0); break;
							case GL_DEPTH_COMPONENT32: fmt = GL_R32F; break;
							default: break;
						}
						bool cube = (tex->flags & TFX_TEXTURE_CUBE) == TFX_TEXTURE_CUBE;
						CHECK(tfx_glBindImageTexture(j, tex->gl_ids[tex->gl_idx], job.textures_mip[j], cube, 0, write ? GL_WRITE_ONLY : GL_READ_ONLY, fmt));
					}
					if (job.ssbos[j].gl_id != 0) {
						tfx_buffer *ssbo = &job.ssbos[j];
						if (ssbo->dirty) {
							CHECK(tfx_glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
							ssbo->dirty = false;
						}
						if (job.ssbo_write[j]) {
							ssbo->dirty = true;
						}
						CHECK(tfx_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, j, job.ssbos[j].gl_id));
					}
					else {
						//CHECK(tfx_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, j, 0));
					}
				}
				update_uniforms(&job);
				CHECK(tfx_glDispatchCompute(job.threads_x, job.threads_y, job.threads_z));
				sb_free(job.uniforms);
				job.uniforms = NULL;
			}
			sb_free(view->jobs);
			view->jobs = NULL;
		}

		// TODO: defer framebuffer creation

		if (canvas->allocated == 0 || nd == 0) {
			continue;
		}

		if (canvas->reconfigure) {
			CHECK(tfx_glBindFramebuffer(GL_FRAMEBUFFER, canvas->gl_fbo[0]));
			canvas_reconfigure(canvas, false);
			canvas->reconfigure = false;
		}

		// TODO: can't render to individual cube face mips with msaa this way
		bool bind_msaa = canvas->msaa && view->canvas_layer <= 0;
		CHECK(tfx_glBindFramebuffer(GL_FRAMEBUFFER, bind_msaa ? canvas->gl_fbo[1] : canvas->gl_fbo[0]));

		if (view->canvas_layer >= 0 && canvas->current_mip != view->canvas_layer && !canvas->cube) {
			int offset = 0;
			for (unsigned i = 0; i < canvas->allocated; i++) {
				tfx_texture *attachment = &canvas->attachments[i];
				canvas->current_width = canvas->width;
				canvas->current_height = canvas->height;
				canvas->current_mip = view->canvas_layer;
				for (int j = 0; j < canvas->current_mip; j++) {
					canvas->current_width /= 2;
					canvas->current_height /= 2;
					canvas->current_width = canvas->current_width > 1 ? canvas->current_width : 1;
					canvas->current_height = canvas->current_height > 1 ? canvas->current_height : 1;
				}
				GLenum attach = GL_DEPTH_ATTACHMENT;
				if (attachment->is_depth && attachment->is_stencil) {
					attach = GL_DEPTH_STENCIL_ATTACHMENT;
				}
				else if (attachment->is_stencil) {
					attach = GL_STENCIL_ATTACHMENT;
				}
				else if (!attachment->is_depth && !attachment->is_stencil) {
					attach = GL_COLOR_ATTACHMENT0 + offset;
					offset += 1;
				}

				assert(canvas->current_mip > 0);
				CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, attachment->gl_ids[0], canvas->current_mip));

				// bind next level for rendering but first restrict fetches only to previous level
				CHECK(tfx_glBindTexture(GL_TEXTURE_2D, attachment->gl_ids[0]));
				CHECK(tfx_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, canvas->current_mip-1));
				CHECK(tfx_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, canvas->current_mip-1));
			}
		}

		if (view->viewport_count == 0) {
			tfx_rect *vp = &view->viewports[0];
			vp->x = 0;
			vp->y = 0;
			vp->w = canvas->current_width;
			vp->h = canvas->current_height;
			view->viewport_count = 1;
		}

		// TODO: render whole view multiple times if this is unavailable?
		if (tfx_glViewportIndexedf) {
			for (int v = 0; v < view->viewport_count; v++) {
				tfx_rect *vp = &view->viewports[v];
				CHECK(tfx_glViewportIndexedf(v, (float)vp->x, (float)vp->y, (float)vp->w, (float)vp->h));
			}
		}
		else {
			tfx_rect *vp = &view->viewports[0];
			CHECK(tfx_glViewport(vp->x, vp->y, vp->w, vp->h));
		}

		// TODO: caps flags for texture array support
		if (view->canvas_layer < 0) {
			assert(canvas->allocated <= 2);
			for (unsigned i = 0; i < canvas->allocated; i++) {
				tfx_texture *attachment = &canvas->attachments[i];
				// TODO: depth stencil
				if (attachment->is_depth) {
					CHECK(tfx_glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, attachment->gl_ids[0], 0));
				}
				else {
					CHECK(tfx_glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, attachment->gl_ids[0], 0));
				}
			}
		}
		else if (canvas->cube) {
			assert(canvas->allocated <= 2);
			for (unsigned i = 0; i < canvas->allocated; i++) {
				tfx_texture *attachment = &canvas->attachments[i];
				if (attachment->is_depth) {
					CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_CUBE_MAP_POSITIVE_X + view->canvas_layer, attachment->gl_ids[0], 0));
				}
				else {
					CHECK(tfx_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + view->canvas_layer, attachment->gl_ids[0], 0));
				}
			}
		}

		if (last_canvas && canvas != last_canvas && last_canvas->gl_fbo[0] != canvas->gl_fbo[0]) {
			GLenum fmt = last_canvas->cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
			if (last_canvas->attachments[0].depth > 1) {
				assert(fmt != GL_TEXTURE_CUBE_MAP); // GL4 & not supported yet
				fmt = GL_TEXTURE_2D_ARRAY;
			}
			for (unsigned i = 0; i < last_canvas->allocated; i++) {
				tfx_texture *attachment = &last_canvas->attachments[i];
				if ((attachment->flags & TFX_TEXTURE_GEN_MIPS) != TFX_TEXTURE_GEN_MIPS) {
					continue;
				}
				tfx_glBindTexture(fmt, attachment->gl_ids[attachment->gl_idx]);
				tfx_glGenerateMipmap(fmt);
			}
		}
		last_canvas = canvas;

		if (view->flags & TFXI_VIEW_SCISSOR) {
			tfx_rect rect = view->scissor_rect;
			CHECK(tfx_glEnable(GL_SCISSOR_TEST));
			CHECK(tfx_glScissor(rect.x, canvas->height - rect.y - rect.h, rect.w, rect.h));
		}
		else {
			CHECK(tfx_glDisable(GL_SCISSOR_TEST));
		}

		GLuint mask = 0;
		if (view->flags & TFXI_VIEW_CLEAR_COLOR) {
			mask |= GL_COLOR_BUFFER_BIT;
			unsigned color = view->clear_color;
			float c[] = {
				((color >> 24) & 0xff) / 255.0f,
				((color >> 16) & 0xff) / 255.0f,
				((color >>  8) & 0xff) / 255.0f,
				((color >>  0) & 0xff) / 255.0f
			};
			CHECK(tfx_glClearColor(c[0], c[1], c[2], c[3]));
			CHECK(tfx_glColorMask(true, true, true, true));
		}

		if (view->flags & TFXI_VIEW_CLEAR_DEPTH) {
			mask |= GL_DEPTH_BUFFER_BIT;
			CHECK(tfx_glClearDepthf(view->clear_depth));
			CHECK(tfx_glDepthMask(true));
		}

		if (mask != 0) {
			CHECK(tfx_glClear(mask));
		}

		if (view->flags & TFXI_VIEW_DEPTH_TEST_MASK) {
			CHECK(tfx_glEnable(GL_DEPTH_TEST));
			if (view->flags & TFXI_VIEW_DEPTH_TEST_LT) {
				CHECK(tfx_glDepthFunc(GL_LEQUAL));
			}
			else if (view->flags & TFXI_VIEW_DEPTH_TEST_GT) {
				CHECK(tfx_glDepthFunc(GL_GEQUAL));
			}
			else if (view->flags & TFXI_VIEW_DEPTH_TEST_EQ) {
				CHECK(tfx_glDepthFunc(GL_EQUAL));
			}
		}
		else {
			CHECK(tfx_glDisable(GL_DEPTH_TEST));
		}

#define CHANGED(diff, mask) ((diff & mask) != 0)

		uint64_t last_flags = 0;
		for (int i = 0; i < nd; i++) {
			tfx_draw draw = view->draws[i];
			if (draw.program != last_program) {
				CHECK(tfx_glUseProgram(draw.program));
				last_program = draw.program;
			}

			// on first iteration of a pass, make sure to set everything.
			if (i == 0) {
				last_flags = ~draw.flags;
			}

			// simple flag diff cuts total GL calls by approx 20% in testing
			uint64_t flags_diff = draw.flags ^ last_flags;
			last_flags = draw.flags;

			if (CHANGED(flags_diff, TFX_STATE_DEPTH_WRITE)) {
				CHECK(tfx_glDepthMask((draw.flags & TFX_STATE_DEPTH_WRITE) == TFX_STATE_DEPTH_WRITE));
			}

			if (CHANGED(flags_diff, TFX_STATE_MSAA) && g_caps.multisample) {
				if (draw.flags & TFX_STATE_MSAA) {
					CHECK(tfx_glEnable(GL_MULTISAMPLE));
				}
				else {
					CHECK(tfx_glDisable(GL_MULTISAMPLE));
				}
			}

			if (CHANGED(flags_diff, TFXI_STATE_CULL_MASK)) {
				if (draw.flags & TFX_STATE_CULL_CW) {
					CHECK(tfx_glEnable(GL_CULL_FACE));
					CHECK(tfx_glFrontFace(GL_CW));
				}
				else if (draw.flags & TFX_STATE_CULL_CCW) {
					CHECK(tfx_glEnable(GL_CULL_FACE));
					CHECK(tfx_glFrontFace(GL_CCW));
				}
				else {
					CHECK(tfx_glDisable(GL_CULL_FACE));
				}
			}

			if (CHANGED(flags_diff, TFXI_STATE_BLEND_MASK)) {
				if (draw.flags & TFXI_STATE_BLEND_MASK) {
					CHECK(tfx_glEnable(GL_BLEND));
					if (draw.flags & TFX_STATE_BLEND_ALPHA) {
						CHECK(tfx_glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
					}
				}
				else {
					CHECK(tfx_glDisable(GL_BLEND));
				}
			}

			if (CHANGED(flags_diff, TFX_STATE_RGB_WRITE) || CHANGED(flags_diff, TFX_STATE_ALPHA_WRITE)) {
				bool write_rgb = (draw.flags & TFX_STATE_RGB_WRITE) == TFX_STATE_RGB_WRITE;
				bool write_alpha = (draw.flags & TFX_STATE_ALPHA_WRITE) == TFX_STATE_ALPHA_WRITE;
				CHECK(tfx_glColorMask(write_rgb, write_rgb, write_rgb, write_alpha));
			}

			if ((view->flags & TFXI_VIEW_SCISSOR) || draw.use_scissor) {
				CHECK(tfx_glEnable(GL_SCISSOR_TEST));
				tfx_rect rect = view->scissor_rect;
				if (draw.use_scissor) {
					rect = draw.scissor_rect;
				}
				CHECK(tfx_glScissor(rect.x, canvas->height - rect.y - rect.h, rect.w, rect.h));
			}
			else {
				CHECK(tfx_glDisable(GL_SCISSOR_TEST));
			}

			update_uniforms(&draw);

			if (draw.callback != NULL) {
				draw.callback();
			}

			if (!draw.use_vbo && !draw.use_ibo) {
				sb_free(draw.uniforms);
				draw.uniforms = NULL;
				continue;
			}

			GLenum mode = GL_TRIANGLES;
			switch (draw.flags & TFXI_STATE_DRAW_MASK) {
				case TFX_STATE_DRAW_POINTS:     mode = GL_POINTS; break;
				case TFX_STATE_DRAW_LINES:      mode = GL_LINES; break;
				case TFX_STATE_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
				case TFX_STATE_DRAW_LINE_LOOP:  mode = GL_LINE_LOOP; break;
				case TFX_STATE_DRAW_TRI_STRIP:  mode = GL_TRIANGLE_STRIP; break;
				case TFX_STATE_DRAW_TRI_FAN:    mode = GL_TRIANGLE_FAN; break;
				default: break; // unspecified = triangles.
			}

			// not available on gles
			if (CHANGED(flags_diff, TFX_STATE_WIREFRAME) && !g_platform_data.use_gles) {
				if (draw.flags & TFX_STATE_WIREFRAME) {
					CHECK(tfx_glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
				}
				else {
					CHECK(tfx_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
				}
			}

			if (draw.use_vbo) {
				GLuint vbo = draw.vbo.gl_id;
	#ifdef TFX_DEBUG
				assert(vbo != 0);
	#endif

				if (draw.vbo.dirty && tfx_glMemoryBarrier) {
					CHECK(tfx_glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT));
					draw.vbo.dirty = false;
				}

				uint32_t va_offset = 0;
				if (draw.use_tvb) {
					draw.vbo.format = draw.tvb_fmt;
					va_offset = draw.offset;
				}
				tfx_vertex_format *fmt = &draw.vbo.format;
				assert(fmt != NULL);
				assert(fmt->stride > 0);

				CHECK(tfx_glBindBuffer(GL_ARRAY_BUFFER, vbo));

				int nc = fmt->count;
	#ifdef TFX_DEBUG
				assert(nc < 8); // the mask is only 8 bits
	#endif

				int real = 0;
				for (int i = 0; i < nc; i++) {
					if ((fmt->component_mask & (1 << i)) == 0) {
						continue;
					}
					tfx_vertex_component vc = fmt->components[i];
					GLenum gl_type = GL_FLOAT;
					switch (vc.type) {
						case TFX_TYPE_SKIP: continue;
						case TFX_TYPE_UBYTE:  gl_type = GL_UNSIGNED_BYTE; break;
						case TFX_TYPE_BYTE:   gl_type = GL_BYTE; break;
						case TFX_TYPE_USHORT: gl_type = GL_UNSIGNED_SHORT; break;
						case TFX_TYPE_SHORT:  gl_type = GL_SHORT; break;
						case TFX_TYPE_FLOAT: break;
						default: assert(0); break;
					}
					if (i >= last_count) {
						CHECK(tfx_glEnableVertexAttribArray(real));
					}
					CHECK(tfx_glVertexAttribPointer(real, (GLint)vc.size, gl_type, vc.normalized, (GLsizei)fmt->stride, (GLvoid*)(vc.offset + va_offset)));
					real += 1;
				}

				last_count = last_count < 0 ? 0 : last_count;
				nc = last_count - nc;
				for (int i = 0; i <= nc; i++) {
					CHECK(tfx_glDisableVertexAttribArray(last_count - i));
				}
				last_count = real;
			}
			else if (last_count > 0) {
				last_count = 0;
				for (int i = 0; i < 8; i++) {
					CHECK(tfx_glDisableVertexAttribArray(i));
				}
			}

			GLuint bind_units[8];
			for (int i = 0; i < 8; i++) {
				tfx_buffer *ssbo = &draw.ssbos[i];
				if (ssbo->gl_id != 0) {
					if (ssbo->dirty) {
						CHECK(tfx_glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
						ssbo->dirty = false;
					}
					if (draw.ssbo_write[i]) {
						ssbo->dirty = true;
					}
					CHECK(tfx_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, draw.ssbos[i].gl_id));
				}

				tfx_texture *tex = &draw.textures[i];
				bool msaa_sample = (tex->flags & TFX_TEXTURE_MSAA_SAMPLE) == TFX_TEXTURE_MSAA_SAMPLE;
				GLuint idx = msaa_sample ? 1 : tex->gl_idx;
				GLuint id = tex->gl_ids[idx];
				bind_units[i] = id;
				if (!g_caps.multibind && id > 0) {
					CHECK(tfx_glActiveTexture(GL_TEXTURE0 + i));

					bool cube = (tex->flags & TFX_TEXTURE_CUBE) == TFX_TEXTURE_CUBE;
					GLenum fmt = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
					if (tex->depth > 1) {
						assert(fmt != GL_TEXTURE_CUBE_MAP);
						fmt = GL_TEXTURE_2D_ARRAY;
					}
					if (msaa_sample) {
						fmt = GL_TEXTURE_2D_MULTISAMPLE;
					}
					CHECK(tfx_glBindTexture(fmt, id));
				}
			}
			if (g_caps.multibind) {
				CHECK(tfx_glBindTextures(0, 8, bind_units));
			}

			int instance_mul = view->instance_mul;
			if (instance_mul == 0) {
				instance_mul = view->viewport_count;
				// canvas layer -1 indicates using layered rendering, multiply instances to suit layer count.
				// for a cubemap array on multiple viewports, this really could be a lot!
				if (view->canvas_layer < 0) {
					if (canvas->cube) {
						instance_mul *= 6;
					}
					if (canvas->attachments[0].depth > 1) {
						instance_mul *= canvas->attachments[0].depth;
					}
				}
			}

			if (draw.use_ibo) {
				if (draw.ibo.dirty && tfx_glMemoryBarrier) {
					CHECK(tfx_glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT));
					draw.ibo.dirty = false;
				}
				CHECK(tfx_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw.ibo.gl_id));
				GLenum index_mode = GL_UNSIGNED_SHORT;
				if ((draw.ibo.flags & TFX_BUFFER_INDEX_32) == TFX_BUFFER_INDEX_32) {
					index_mode = GL_UNSIGNED_INT;
				}
				CHECK(tfx_glDrawElementsInstanced(mode, draw.indices, index_mode, (GLvoid*)draw.offset, 1*instance_mul));
			}
			else {
				CHECK(tfx_glDrawArraysInstanced(mode, 0, (GLsizei)draw.indices, 1*instance_mul));
			}

			sb_free(draw.uniforms);
			draw.uniforms = NULL;
		}

#undef CHANGED

		sb_free(view->jobs);
		view->jobs = NULL;

		sb_free(view->draws);
		view->draws = NULL;

		sb_free(view->blits);
		view->blits = NULL;
	}

	pop_group();

	if (use_timers) {
		// record the finishing time so we can figure out the last view timing
		CHECK(tfx_glQueryCounter(g_timers[VIEW_MAX + g_timer_offset], GL_TIMESTAMP));
	}

	if (use_timers && stats.num_timings > 0) {
		int idx = VIEW_MAX + next_offset;
		GLuint result_available = 0;
		CHECK(tfx_glGetQueryObjectuiv(g_timers[idx], GL_QUERY_RESULT_AVAILABLE, &result_available));
		if (result_available) {
			GLuint64 result = 0;
			CHECK(tfx_glGetQueryObjectui64v(g_timers[idx], GL_QUERY_RESULT, &result));
			stats.timings[stats.num_timings-1].time = result - last_result;
		}
	}

	g_timer_offset = next_offset;

	reset();

	tvb_reset();

	sb_free(g_back.uniforms);
	g_back.uniforms = NULL;

	g_back.ub_cursor = g_back.uniform_buffer;

	CHECK(tfx_glDisable(GL_SCISSOR_TEST));
	CHECK(tfx_glColorMask(true, true, true, true));

	if (tfx_glDeleteVertexArrays) {
		CHECK(tfx_glDeleteVertexArrays(1, &vao));
	}

	// shift buffers to avoid stalls
	tfx_buffer tmp = g_transient_buffer.buffers[0];
	for (int i = 0; i < TFX_TRANSIENT_BUFFER_COUNT - 1; i++) {
		g_transient_buffer.buffers[i] = g_transient_buffer.buffers[i+1];
	}
	g_transient_buffer.buffers[TFX_TRANSIENT_BUFFER_COUNT-1] = tmp;

	if ((g_flags & TFX_RESET_DEBUG_OVERLAY_STATS) == TFX_RESET_DEBUG_OVERLAY_STATS) {
		int row = 0;

		uint16_t color[2] = { 0x140f, 0x160f };

		const char *str = tfx_sprintf("Draws: %5d", stats.draws);
		int lrow = row; row++; // avoid warning from -Wunsequenced
		tfx_debug_print(lrow, 0, color[row % 2], 0, str);
		free((char*)str);

		lrow = row; row++;
		str = tfx_sprintf("Blits: %5d", stats.blits);
		tfx_debug_print(lrow, 0, color[row % 2], 0, str);
		free((char*)str);

		int max_width = 0;
		for (unsigned i = 0; i < stats.num_timings; i++) {
			int len = strnlen(stats.timings[i].name, 100);
			if (len > max_width) {
				max_width = len;
			}
		}
		uint64_t sum = 0;
		for (unsigned i = 0; i < stats.num_timings; i++, row++) {
			sum += stats.timings[i].time;
			str = stats.timings[i].name;
			int width = 5 + max_width;
			tfx_debug_print(row, width - strnlen(str, 100), color[row % 2], 0, str);

			str = tfx_sprintf("%3d", stats.timings[i].id);
			tfx_debug_print(row, 0, color[row % 2], 0, str);
			free((char*)str);

			str = tfx_sprintf("%dus", stats.timings[i].time / 1000);
			tfx_debug_print(row, 8 + width - strnlen(str, 20), color[row % 2], 0, str);
			free((char*)str);
		}
		tfx_debug_print(row, 0, 0x100f, 0, "Total:");
		str = tfx_sprintf("%dus", sum / 1000);
		tfx_debug_print(row, 13 + max_width - strnlen(str, 20), 0x100f, 0, str);
		free((char*)str);
	}

	return stats;
}
#undef MAX_VIEW
#undef CHECK

#undef TFX_INFO
#undef TFX_WARN
#undef TFX_ERROR
#undef TFX_FATAL

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifdef __cplusplus
}
#endif

#endif // TFX_IMPLEMENTATION
