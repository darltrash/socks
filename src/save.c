#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASKET_INTERNAL
#include "common.h"

static SDL_RWops *ops = NULL;

int sav_identity(const char *identity) {
    const char *path = SDL_GetPrefPath("BASKET", identity);
    
    char *full_path = malloc(strlen(path) + 4);
    strcpy(full_path, path);
    strcat(full_path, "sv0");

    ops = SDL_RWFromFile(full_path, "r+b");

    if (!ops)
        ops = SDL_RWFromFile(full_path, "w+b");

    return 0;
}

int sav_store(const char *data, size_t length) {
    if (!ops) 
        return 1;
    
    ops->seek(ops, 0, RW_SEEK_SET);
    ops->write(ops, data, 1, length);

    return 0;
}

// You own the memory that comes out of this thing.
char *sav_retrieve(u32 *length) {
    if (!ops) 
        return NULL;

    u64 size = ops->size(ops);
    *length = size;

    char *data = malloc(size);
    ops->seek(ops, 0, RW_SEEK_SET);
    ops->read(ops, data, 1, size);

    return data;
}