// implement the simplest freaking little filesystem system thing

#include "common.h"
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