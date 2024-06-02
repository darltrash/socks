// implement the simplest freaking little filesystem system thing

#define BASKET_INTERNAL
#include "basket.h"
#include <stdio.h>

#ifdef FS_NAIVE_FILES
    #include <stdlib.h>

    // This code is bound to leak:
    const char *fs_read(const char *name, u32 *size) {
        printf("reading file %s\n", name);

        char real_filepath[50] = "package/";
        strcat(real_filepath, name);

        FILE *f = fopen(real_filepath, "rb");
        if (!f)
            return 0;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (size)
            *size = (u32)fsize;

        char *string = malloc(fsize + 1);
        fread(string, fsize, 1, f);
        fclose(f);

        string[fsize] = 0;

        return string;
    }

#else
    #include "static.h"

    const char *fs_read(const char *name, u32 *size) {
        //printf("reading file %s\n", name);

        u8 a = 0;

        do {
            if (strcmp(fs_table[a].filename, name) == 0) {
                if (size != NULL)
                    *size = fs_table[a].size; 
                
                return (char *)fs_table[a].data;
            }
        } while (fs_table[++a].filename);

        return 0;
    }
#endif


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

int sav_store(const char *data, u32 length) {
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