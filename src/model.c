// implement a tiny iqm loader, it also has an EXM section.
// it can only understand a super specific version of IQM files 
// because I cant bother to make it generalized.

#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

typedef struct {
    char magic[16];
    unsigned int version;
    unsigned int filesize;
    unsigned int flags;
    unsigned int num_text, ofs_text;
    unsigned int num_meshes, ofs_meshes;
    unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
    unsigned int num_triangles, ofs_triangles, ofs_adjacency;
    unsigned int num_joints, ofs_joints;
    unsigned int num_poses, ofs_poses;
    unsigned int num_anims, ofs_anims;
    unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
    unsigned int num_comment, ofs_comment;
    unsigned int num_extensions, ofs_extensions;
} iqmheader;

enum
{
    IQM_POSITION     = 0,
    IQM_TEXCOORD     = 1,
    IQM_NORMAL       = 2,
    IQM_TANGENT      = 3,
    IQM_BLENDINDEXES = 4,
    IQM_BLENDWEIGHTS = 5,
    IQM_COLOR        = 6,
    IQM_CUSTOM       = 0x10
};

enum
{
    IQM_BYTE   = 0,
    IQM_UBYTE  = 1,
    IQM_SHORT  = 2,
    IQM_USHORT = 3,
    IQM_INT    = 4,
    IQM_UINT   = 5,
    IQM_HALF   = 6,
    IQM_FLOAT  = 7,
    IQM_DOUBLE = 8
};

typedef struct  {
    unsigned int type;
    unsigned int flags;
    unsigned int format;
    unsigned int size;
    unsigned int offset;
} iqmvertexarray;

bool iqm_load(Map *map, const char *data) {
    iqmheader header = *(iqmheader *)data;

    // Check data
    if (strcmp(header.magic, IQM_MAGIC)) {
        return true;
    }

    // Find our data (does a lot of assumptions but who the fuck cares.)
    f32 *positions = NULL;
    Color *colors = NULL;
    f32 *uvs = NULL;

    iqmvertexarray *vertex_arrays = (iqmvertexarray *)(data+header.ofs_vertexarrays);
    for (u32 i = 0; i < header.num_vertexarrays; i++) {
        iqmvertexarray va = vertex_arrays[i];
        switch (va.type) {
            case IQM_POSITION: {
                positions = (f32 *)(data+va.offset);
                break;
            }

            case IQM_TEXCOORD: {
                uvs = (f32 *)(data+va.offset);
                break;
            }

            case IQM_COLOR: {
                colors = (Color *)(data+va.offset);
                break;
            }

            default: break;
        }
    }

    // Assemble data
    u16 len = header.num_triangles * 3;
    Vertex *vertices = falloc(Vertex, len);

    u32 *indices = (u32 *)(data+header.ofs_triangles);
    for (u32 i=0; i < len; i++) {
        u32 k = indices[i];
        
        Vertex o = {
            { 0.f, 0.f, 0.f }, 
            { 0.f, 0.f }, 
            { .full = 0xFFFFFFFF }
        };

        if (positions != NULL)
            memcpy(o.position, &positions[k*3], sizeof(f32)*3);

        if (uvs != NULL)
            memcpy(o.uv, &uvs[k*2], sizeof(f32)*2);

        if (colors != NULL) {
            o.color = (Color) {
                .r = colors[k].a,
                .g = colors[k].b,
                .b = colors[k].g,
                .a = colors[k].r,
            };

            //o.color.r -= (u8)(rand()) % 8;
            //o.color.g -= (u8)(rand()) % 8;
            //o.color.b -= (u8)(rand()) % 8;
        }

        vertices[i] = o;
    }

    RenderBox box = {
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}
    };

    char *exm = NULL;

    if (header.num_comment)
        map->extra = (char *)data+header.ofs_comment;

    map->mesh = (MeshSlice) {
        vertices, len, box
    };

    //free(vertices);

    return false;
}


#define BBM_MAGIC "BASKETMODELv0.1"

typedef struct {
    char magic[16]; // BASKETMODELv0.1\0

    u32 vertex_amount;
    u16 vertex_offset;
    u32 extra_amount; // if 0, ignore
    u32 extra_offset;
} BasketModelHeader;

bool bbm_load(Map *map, const char *data) {
    BasketModelHeader header = *(BasketModelHeader*)(data);

    if (strcmp(header.magic, "BASKETMODELv0.1")) {
        return true;
    }

    u64 size = sizeof(Vertex) * header.vertex_amount;
    map->mesh.data = malloc(size);
    memcpy(map->mesh.data, data + header.vertex_offset, size);

    if (header.extra_amount) {
        map->extra = malloc(header.extra_amount);
        memcpy(map->extra, data + header.extra_offset, header.extra_amount);
    }
    
    return false;
}


Map mod_load(const char *data) {
    Map map = (Map) { { 0, 0 }, 0 };

    if (!iqm_load(&map, data)) {
        return map;
    }

    if (!bbm_load(&map, data)) {
        return map;
    }

    return map;
}