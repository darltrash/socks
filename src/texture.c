#include "common.h"
#include "stb_image.h"

Texture tex_load(const char *data, u32 length) {
    Texture tex = { NULL, 0, 0 };

    int w, h, c;

    // TODO: HANDLE OTHER CASES :O
    tex.pixels = (Color *)stbi_load_from_memory((const stbi_uc *)data, length, &w, &h, &c, 4);
    tex.w = w;
    tex.h = h;

    if (!tex.pixels) {
        printf("tex_load error: %s\n", stbi_failure_reason());

        return (Texture) { NULL, 0, 0 };
    }

    return tex;
}

void tex_free(Texture texture) {
    if (texture.pixels) {
        stbi_image_free(texture.pixels);
    }
}