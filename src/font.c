// Can only load TTF as meshes

#include "basket.h"
#include "ttf2mesh.h"
#include "vec.h"

int fnt_init_ttf(Font *font, const char *data, u32 length, u32 quality) {
    if (quality == 0)
        quality = 10;

    ttf_t *ttf;
    if (ttf_load_from_mem((void *)data, length, &ttf, false) != TTF_DONE)
        return 1;

    Character *characters = malloc(sizeof(Character) * ttf->nchars);

    font->character_amount = 0;

    for (int i = 0; i < ttf->nchars; i++) {
        ttf_glyph_t *glyph = ttf->glyphs + ttf->char2glyph[i];

        ttf_mesh_t *mesh;
        if (ttf_glyph2mesh(glyph, &mesh, quality, 0) != TTF_DONE)
            continue;

        Character c = {
            .mesh = (MeshSlice) {
                .animation = NULL,
                .data = malloc(sizeof(Vertex) * mesh->nfaces * 3),
                .length = mesh->nfaces * 3
            },
            
            .codepoint = ttf->chars[i],
            .advance = glyph->advance,
        };

        if (ttf->chars[i] == '?') 
            font->fallback = i;

        for (int j = 0; j < mesh->nfaces; j++) {
            c.mesh.data[(j*3)+0] = (Vertex) {
                { mesh->vert[mesh->faces[j].v1].x, 1.0-mesh->vert[mesh->faces[j].v1].y, 0.0f },
                { mesh->vert[mesh->faces[j].v1].x, 1.0-mesh->vert[mesh->faces[j].v1].y },
                { 255, 255, 255, 255 }
            };

            c.mesh.data[(j*3)+1] = (Vertex) {
                { mesh->vert[mesh->faces[j].v2].x, 1.0-mesh->vert[mesh->faces[j].v2].y, 0.0f },
                { mesh->vert[mesh->faces[j].v2].x, 1.0-mesh->vert[mesh->faces[j].v2].y },
                { 255, 255, 255, 255 }
            };

            c.mesh.data[(j*3)+2] = (Vertex) {
                { mesh->vert[mesh->faces[j].v3].x, 1.0-mesh->vert[mesh->faces[j].v3].y, 0.0f },
                { mesh->vert[mesh->faces[j].v3].x, 1.0-mesh->vert[mesh->faces[j].v3].y },
                { 255, 255, 255, 255 }
            };
        }

        characters[font->character_amount++] = c;

        ttf_free_mesh(mesh);
    }

    characters = realloc(characters, font->character_amount * sizeof(Character));

    ttf_free(ttf);

    font->characters = characters;

    return 0;
}

void fnt_free(Font *font) {
    for (int i=0; i < font->character_amount; i++)
        free(font->characters[i].mesh.data);

    free(font->characters);
}