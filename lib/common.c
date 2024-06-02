// we will only use PNGs on this current project.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_GIF
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

// we need this for OGG files.
// no STDIO because we will not read directly off disk
// no PUSHDATA because we will not export OGG files.
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.h"

// we force SDL2 here because we already have SDL2 as the base of the program
// no matter what platform, and keeps things consistent.
#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#define CUTE_SOUND_SDL_H <SDL2/SDL.h>
#ifndef __SSE__
#define CUTE_SOUND_SCALAR_MODE 1
#endif
#include "cute_sound.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
