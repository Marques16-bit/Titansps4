#include "gamescan.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dirlist.h"

#define MAX_DEPTH 8
#define MAX_FILES 60000

typedef struct {
    GameReport* r;
    char path[240];
    int depth;
    char shipping[240];     /* *-Win64-Shipping.exe */
    char first_exe[240];    /* primeiro .exe encontrado (o mais raso) */
    int first_exe_depth;
} Walk;

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int ends_ci(const char* s, const char* suffix) {
    size_t n = strlen(s), m = strlen(suffix);
    if (n < m) return 0;
    for (size_t i = 0; i < m; i++) if (lc(s[n - m + i]) != lc(suffix[i])) return 0;
    return 1;
}

static int64_t file_size(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    off_t n = lseek(fd, 0, SEEK_END);
    close(fd);
    return (int64_t)n;
}

static void walk_dir(Walk* w);

static void on_entry(const char* name, int is_dir, void* ud) {
    Walk* w = (Walk*)ud;
    if (w->r->files >= MAX_FILES) return;
    size_t base = strlen(w->path);
    if (base + strlen(name) + 2 >= sizeof w->path) return;
    w->path[base] = '/';
    strcpy(w->path + base + 1, name);
    if (is_dir) {
        if (w->depth < MAX_DEPTH) {
            w->depth++;
            walk_dir(w);
            w->depth--;
        }
    } else {
        GameReport* r = w->r;
        int64_t sz = file_size(w->path);
        r->files++;
        if (sz > 0) r->total_bytes += sz;
        if (ends_ci(name, ".pak") || ends_ci(name, ".utoc") || ends_ci(name, ".ucas")) {
            r->pak_files++;
            if (sz > 0) r->pak_bytes += sz;
        }
        if (ends_ci(name, ".dll")) r->dll_files++;
        if (ends_ci(name, ".exe")) {
            r->exe_files++;
            if (ends_ci(name, "-win64-shipping.exe") && !w->shipping[0]) snprintf(w->shipping, sizeof w->shipping, "%s", w->path);
            if (!w->first_exe[0] || w->depth < w->first_exe_depth) {
                snprintf(w->first_exe, sizeof w->first_exe, "%s", w->path);
                w->first_exe_depth = w->depth;
            }
        }
    }
    w->path[base] = 0;
}

static void walk_dir(Walk* w) {
    if (dir_each(w->path, on_entry, w) != 0 && w->depth == 0) w->r->dir_ok = 0;
}

static void write_str(int fd, const char* fmt, ...) {
    char line[400];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n > 0) write(fd, line, (size_t)(n > (int)sizeof line - 1 ? (int)sizeof line - 1 : n));
}

/* le "exe=relativo" do game.ini */
static int read_override(const char* ini_path, char* out, size_t cap) {
    char buf[512];
    int fd = open(ini_path, O_RDONLY);
    if (fd < 0) return 0;
    int n = (int)read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = 0;
    for (char* p = buf; *p;) {
        char* nl = strchr(p, '\n');
        if (nl) *nl = 0;
        if (!strncmp(p, "exe=", 4)) {
            char* v = p + 4;
            size_t len = strlen(v);
            while (len && (v[len - 1] == '\r' || v[len - 1] == ' ')) v[--len] = 0;
            snprintf(out, cap, "%s", v);
            return out[0] != 0;
        }
        if (!nl) break;
        p = nl + 1;
    }
    return 0;
}

void gamescan_run(GameReport* r, const char* game_dir, const char* ini_path, const char* report_path) {
    memset(r, 0, sizeof *r);
    r->dir_ok = 1;
    static Walk w;
    memset(&w, 0, sizeof w);
    w.r = r;
    snprintf(w.path, sizeof w.path, "%s", game_dir);
    walk_dir(&w);

    char rel[200];
    if (read_override(ini_path, rel, sizeof rel)) {
        snprintf(r->exe, sizeof r->exe, "%s/%s", game_dir, rel);
        snprintf(r->exe_source, sizeof r->exe_source, "game.ini");
    } else if (w.shipping[0]) {
        snprintf(r->exe, sizeof r->exe, "%s", w.shipping);
        snprintf(r->exe_source, sizeof r->exe_source, "auto (Shipping)");
    } else if (w.first_exe[0]) {
        snprintf(r->exe, sizeof r->exe, "%s", w.first_exe);
        snprintf(r->exe_source, sizeof r->exe_source, "auto (primeiro .exe)");
    }

    int fd = open(report_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write_str(fd, "Titans of the Tide (PS4) - relatorio da analise do jogo\n");
        write_str(fd, "pasta=%s listagem=%s arquivos=%d bytes=%lld pak/utoc/ucas=%d (%lld bytes) exe=%d dll=%d\n", game_dir,
                  r->dir_ok ? "ok" : "indisponivel", r->files, (long long)r->total_bytes, r->pak_files, (long long)r->pak_bytes,
                  r->exe_files, r->dll_files);
        write_str(fd, "executavel=%s origem=%s\n\n", r->exe[0] ? r->exe : "(nao encontrado)", r->exe_source);
    }
    if (r->exe[0]) pe_analyze(r->exe, &r->pe, fd);
    else snprintf(r->pe.error, sizeof r->pe.error, "nenhum .exe encontrado");
    if (fd >= 0) close(fd);
    r->done = 1;
}
