#include "dirlist.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#if defined(TITANS_HOST)
/* Build do PC (testes): opendir normal. */
#include <dirent.h>

int dir_each(const char* path, DirCb cb, void* ud) {
    DIR* d = opendir(path);
    if (!d) return -1;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        cb(e->d_name, e->d_type == DT_DIR, ud);
    }
    closedir(d);
    return 0;
}

#elif defined(HAVE_KERNEL_GETDENTS)
/* PS4: sceKernelGetdents devolve o struct dirent do FreeBSD: fileno(4) reclen(2) type(1) namlen(1) name[]. */
extern int ti_getdents(int fd, char* buf, int nbytes) __asm__("sceKernelGetdents");

#define DT_DIR_BSD 4

int dir_each(const char* path, DirCb cb, void* ud) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    char buf[4096];
    int n;
    while ((n = ti_getdents(fd, buf, sizeof buf)) > 0) {
        for (int off = 0; off + 8 <= n;) {
            const unsigned char* e = (const unsigned char*)buf + off;
            unsigned fileno = e[0] | (e[1] << 8) | (e[2] << 16) | ((unsigned)e[3] << 24);
            unsigned reclen = e[4] | (e[5] << 8);
            unsigned type = e[6];
            unsigned namlen = e[7];
            if (reclen < 8 || off + (int)reclen > n) break;
            if (fileno && namlen && namlen < 256) {
                char name[256];
                memcpy(name, e + 8, namlen);
                name[namlen] = 0;
                if (strcmp(name, ".") && strcmp(name, "..")) cb(name, type == DT_DIR_BSD, ud);
            }
            off += (int)reclen;
        }
    }
    close(fd);
    return n < 0 ? -1 : 0;
}

#else
int dir_each(const char* path, DirCb cb, void* ud) {
    (void)path; (void)cb; (void)ud;
    return -1;
}
#endif
