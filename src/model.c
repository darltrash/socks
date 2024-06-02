// implement a tiny iqm loader, it also has an EXM section.
// it can only understand a super specific version of IQM files 
// because I cant bother to make it generalized.

#define BASKET_INTERNAL
#include "basket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} IQMHeader;


typedef struct {
  unsigned int name;
  int parent;
  Transform transform;
} IQMJoint;


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
} IQMVertexArray;

typedef struct {
  int parent;
  unsigned int channelmask;
  f32 channeloffset[10];
  f32 channelscale[10];
} IQMPose;

typedef struct {
  unsigned int name;
  unsigned int first_frame, num_frames;
  f32 framerate;
  unsigned int flags;
} IQMAnim;

bool iqm_load(Model *map, const char *data) {
    IQMHeader header = *(IQMHeader *)data;

    // Check data
    if (strcmp(header.magic, IQM_MAGIC)) {
        return true;
    }

    const char *text = header.ofs_text ? &data[header.ofs_text] : "";

    // Find our data (does a lot of assumptions but who the fuck cares.)
    f32 *positions = NULL;
    Color *colors = NULL;
    f32 *uvs = NULL;
    u8 *blend_indices = NULL;
    u8 *blend_weight = NULL;

    IQMVertexArray *vertex_arrays = (IQMVertexArray *)(data+header.ofs_vertexarrays);
    for (u32 i = 0; i < header.num_vertexarrays; i++) {
        IQMVertexArray va = vertex_arrays[i];
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

            case IQM_BLENDINDEXES: {
                blend_indices = (u8 *)(data+va.offset);
                break;
            }

            case IQM_BLENDWEIGHTS: {
                blend_weight = (u8 *)(data+va.offset);
                break;
            }

            default: break;
        }
    }

    RenderBox box = {
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}
    };

    // Assemble data
    u16 vertex_amount = header.num_triangles * 3;
    Vertex *vertices = falloc(Vertex, vertex_amount);
    VertexAnim *animdata = NULL;

    u32 *indices = (u32 *)(data+header.ofs_triangles);
    for (u32 i=0; i < vertex_amount; i++) {
        u32 k = indices[i];
        
        Vertex o = {
            { 0.f, 0.f, 0.f }, 
            { 0.f, 0.f }, 
            { .full = 0xFFFFFFFF }
        };

        if (positions != NULL) {
            memcpy(o.position, &positions[k*3], sizeof(f32)*3);

            for (int i=0; i < 3; i++)
                box.min[i] = min(box.min[i], o.position[i]);
                        
            for (int i=0; i < 3; i++)
                box.max[i] = max(box.max[i], o.position[i]);
        }

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

    if (header.num_joints && blend_indices) {
        animdata = falloc(VertexAnim, vertex_amount);

        for (u32 i=0; i < vertex_amount; i++) {
            u32 k = indices[i];

            memcpy(animdata[i].bone,   &blend_indices[k*4],4);
            memcpy(animdata[i].weight, &blend_weight[k*4], 4);
        }

        map->animation.bones     = malloc(sizeof(Bone)     *header.num_joints);
        map->animation.bind_pose = malloc(sizeof(Transform)*header.num_joints);
        map->animation.pose      = malloc(sizeof(Transform)*header.num_joints);

        IQMJoint *joints  = (IQMJoint *)(data+header.ofs_joints);
        map->animation.bone_amount = header.num_joints;

        for (int i = 0; i < header.num_joints; i++) {
            const IQMJoint j = joints[i];

            strncpy(map->animation.bones[i].name, &text[j.name], 64);

            map->animation.bones[i].transform = j.transform;
            map->animation.bones[i].parent = j.parent;
            map->animation.bind_pose[i] = j.transform;
        }

        if (header.num_anims) {
            IQMAnim *rawanim = (IQMAnim *)&data[header.ofs_anims];
            
            map->animation.animation_amount = header.num_anims;
            map->animation.animations = malloc(sizeof(Animation)*header.num_anims);

            for (int i=0; i<header.num_anims; i++) {
                IQMAnim a = rawanim[i];

                Animation o = {
                    .name  = strdup(text + a.name),
                    .first = a.first_frame,
                    .last  = a.num_frames,
                    .rate  = a.framerate,
                    .loops = a.flags,
                };

                map->animation.animations[i] = o;
            }  
        }

        if (header.num_frames) {
            IQMPose *posedata = (IQMPose *)&data[header.ofs_poses];

            map->animation.frames = malloc(sizeof(AnimationFrame)*header.num_frames);
            unsigned short *framedata = (unsigned short *)&data[header.ofs_frames];
            
            for (int i=0; i<header.num_frames; i++) {
                Transform *frame = malloc(sizeof(Transform)*header.num_poses);
                
                for (int p=0; p<header.num_poses; p++) {
                    IQMPose *pose = &posedata[p];
                    
                    f32 *v = (f32 *)(&frame[p]);

                    for (int o=0; o<10; o++) {
                        f32 val = pose->channeloffset[o];

                        unsigned int mask = (1 << o);

                        if ((pose->channelmask & mask) > 0) {
                            val += (*framedata) * pose->channelscale[o];
                            framedata++;
                        }

                        v[o] = val;
                    }
                }
                
                map->animation.frames[i] = frame;
            }
        }
    }

    char *exm = NULL;

    if (header.num_comment)
        map->extra = (char *)data+header.ofs_comment;

    map->mesh = (MeshSlice) {
        vertices, animdata, vertex_amount, box
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

bool bbm_load(Model *map, const char *data) {
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


int mod_load(const char *data, Model *map) {
    if (!iqm_load(map, data)) {
        return 0;
    }

    if (!bbm_load(map, data)) {
        return 0;
    }

    return 1;
}

void mod_free(Model *model) {
    if (model->mesh.data)
        free(model->mesh.data);

    if (model->mesh.animation)
        free(model->mesh.animation);

    model->mesh = (MeshSlice) { NULL, NULL, 0, {} };


    // TODO: FREE ANIMATION
}
