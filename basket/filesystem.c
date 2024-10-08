// implement the simplest freaking little filesystem system thing

#include <SDL2/SDL_rwops.h>
#define BASKET_INTERNAL
#include "basket.h"
#include <stdio.h>
#include <dirent.h>

#ifdef _WIN32
    #include <windows.h>
    #include <libloaderapi.h>
    #define PATH_SEPARATOR '\\'
    #define chdir(path) (!SetCurrentDirectory(path))
#elif __APPLE__
    #include <mach-o/dyld.h>
#else
    #define PATH_SEPARATOR '/'
    #include <unistd.h>
#endif

#ifdef WIN32
    #define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif

static int chdir_to_path(const char *exec_path) {
    char resolved_path[1024];

    #ifdef _WIN32
        GetModuleFileNameA(NULL, resolved_path, sizeof(resolved_path));

    #elif __linux__
        size_t s = readlink("/proc/self/exe", resolved_path, sizeof(resolved_path));
        resolved_path[s] = 0;

    #elif __FreeBSD__
        size_t s = readlink("/proc/curproc/file", resolved_path, sizeof(resolved_path));
        resolved_path[s] = 0;

    #elif __APPLE__
        size_t s = 0;
        _NSGetExecutablePath(resolved_path, &s);

    #else
        if (realpath(exec_path, resolved_path) == NULL)
            return 1;

    #endif

    char *last_slash = strrchr(resolved_path, PATH_SEPARATOR);

    if (last_slash == NULL)
        return 1;

    *last_slash = '\0';

    return chdir(resolved_path);
}

#ifdef FS_STATIC_FILES
    #include "static.h"

    char *fs_read(const char *name, u32 *size) {
        //printf("reading file %s\n", name);

        u8 a = 0;

        do {
            if (strcmp(fs_table[a].filename, name) == 0) {
                if (size != NULL)
                    *size = fs_table[a].size;

                return strdup(fs_table[a].data);
            }
        } while (fs_table[++a].filename);

        return 0;
    }

    int fs_init(const char *self) {
        return 0;
    }

#else
    #include "lib/zip.h"

    struct zip_t *zip;

    #define FOUND(where, type) {\
        printf("found assets in '%s' (%s)!\n", where, type);\
        return 0;\
    }

    int fs_init(const char *self) {
        if (chdir_to_path(self))
            return 1;

        // Check if BASKET_PACKAGE is not NULL
        const char *name = getenv("BASKET_PACKAGE");

        if (name != NULL) {
            // If $BASKET_PACKAGE is a valid directory we can CD into
            if (!chdir(name))
                FOUND(name, "folder");

            // Try loading a zip file at $BASKET_PACKAGE
            zip = zip_open(name, 0, 'r');
            if (zip)
                FOUND(name, "bsk")
        }

        // If ./package/ is a valid directory we can CD into
        if (!chdir("package"))
            FOUND("package", "folder");

        // Try loading a zip file at ./package.bsk
        zip = zip_open("package.bsk", 0, 'r');
        if (zip)
            FOUND("package.bsk", "bsk");

        printf("no data package could be found! Are you sure you didn't delete anything?\n");

        return 1; // We couldn't find anything :/
    }

    static char *fs_read_zip(const char *name, u32 *size) {
        char *buffer = NULL;
        size_t length;

        zip_entry_open(zip, name);

        int ret = zip_entry_read(zip, (void**)(&buffer), &length);

        zip_entry_close(zip);

        if (ret < 0)
            return NULL;

        if (size != NULL)
            *size = length;

        return buffer;
    }

    static char *fs_read_folder(const char *name, u32 *size) {
        printf("reading file %s\n", name);

        FILE *f = fopen(name, "rb");
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

    char *fs_read(const char *name, u32 *size) {
        if (zip)
            return fs_read_zip(name, size);

        return fs_read_folder(name, size);
    }

    void fs_byebye() {
        if (zip)
            zip_close(zip);
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

void sav_byebye() {
    if (ops)
        SDL_RWclose(ops);
}
