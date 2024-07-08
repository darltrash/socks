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

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
