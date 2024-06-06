#include "basket.h"
#include "stb_image.h"

int img_init(Image *texture, const char *data, u32 length) {
    int w, h, c;

    // TODO: HANDLE OTHER CASES(? :O
    texture->pixels = (Color *)stbi_load_from_memory (
        (const stbi_uc *)data, length, &w, &h, &c, 4
    );
    
    texture->w = w;
    texture->h = h;

    if (!texture->pixels) {
        printf("tex_load error: %s\n", stbi_failure_reason());

        return 1;
    }

    return 0;
}

void img_free(Image *texture) {
    if (texture->pixels) {
        stbi_image_free(texture->pixels);
    }

    texture->pixels = NULL;
    texture->w = 0;
    texture->h = 0;
}