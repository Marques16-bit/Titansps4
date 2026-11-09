#include "pe.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct { uint32_t va, vsize, raw, rawsize, flags; char name[9]; } Sec;

static Sec g_sec[96];
static int g_nsec;
static int g_fd, g_rep;
static uint32_t g_hdr_size;
static PeInfo* g_o;
static uint8_t g_blk[65536 + 64];

#define SEC_EXEC 0x20000000u
#define SEC_READ 0x40000000u
#define SEC_WRITE 0x80000000u

static uint16_t u16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t u32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t u64(const uint8_t* p) { return u32(p) | ((uint64_t)u32(p + 4) << 32); }

static int read_at(int64_t off, void* buf, size_t n) {
    if (lseek(g_fd, (off_t)off, SEEK_SET) < 0) return -1;
    ssize_t r = read(g_fd, buf, n);
    return r < 0 ? -1 : (int)r;
}

static void rep(const char* fmt, ...) {
    if (g_rep < 0) return;
    char line[320];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int)sizeof line - 1) n = (int)sizeof line - 1;
    write(g_rep, line, (size_t)n);
}

static void fail(const char* msg) {
    g_o->ok = 0;
    snprintf(g_o->error, sizeof g_o->error, "%s", msg);
}

static int64_t rva_to_off(uint32_t rva) {
    if (rva < g_hdr_size) return rva;
    for (int i = 0; i < g_nsec; i++) {
        const Sec* s = &g_sec[i];
        uint32_t lim = s->vsize && s->vsize < s->rawsize ? s->vsize : s->rawsize;
        if (rva >= s->va && rva - s->va < lim) return (int64_t)s->raw + (rva - s->va);
    }
    return -1;
}

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int starts_ci(const uint8_t* p, size_t avail, const char* needle) {
    size_t n = strlen(needle);
    if (avail < n) return 0;
    for (size_t i = 0; i < n; i++) if (lc((char)p[i]) != needle[i]) return 0;
    return 1;
}

typedef struct { const char* needle; size_t off; } Hint;
#define H(f) offsetof(PeInfo, f)
static const Hint k_hints[] = {
    {"d3d12", H(hint_d3d12)}, {"d3d11", H(hint_d3d11)}, {"vulkan-1", H(hint_vulkan)}, {"opengl32", H(hint_opengl)},
    {"dxcompiler", H(hint_dxc)}, {"steam_api", H(hint_steam)}, {"eossdk", H(hint_eos)}, {"oo2core", H(hint_oodle)},
    {"bink2w64", H(hint_bink)}, {"binkawin64", H(hint_bink)}, {"fmod", H(hint_fmod)},
};
#define NHINTS ((int)(sizeof k_hints / sizeof k_hints[0]))

static void mark_text(const uint8_t* p, size_t avail) {
    for (int i = 0; i < NHINTS; i++)
        if (starts_ci(p, avail, k_hints[i].needle)) *(int*)((char*)g_o + k_hints[i].off) = 1;
}

static void mark_name(const char* name) {
    size_t n = strlen(name);
    for (size_t i = 0; i < n; i++) mark_text((const uint8_t*)name + i, n - i);
}

/* procura dicas em strings das secoes so de leitura (.rdata) */
static void scan_strings(void) {
    unsigned char first[256];
    memset(first, 0, sizeof first);
    for (int i = 0; i < NHINTS; i++) first[(unsigned char)k_hints[i].needle[0]] = 1;
    int64_t budget = 192ll << 20;
    for (int si = 0; si < g_nsec; si++) {
        const Sec* s = &g_sec[si];
        if (!(s->flags & SEC_READ) || (s->flags & (SEC_EXEC | SEC_WRITE)) || s->rawsize == 0) continue;
        if (s->rawsize > (128u << 20) || budget < (int64_t)s->rawsize) continue;
        budget -= s->rawsize;
        for (uint32_t pos = 0; pos < s->rawsize; pos += 65536) {
            size_t want = s->rawsize - pos < 65536 + 32 ? s->rawsize - pos : 65536 + 32;
            int got = read_at((int64_t)s->raw + pos, g_blk, want);
            if (got <= 0) break;
            size_t lim = got > 65536 ? 65536 : (size_t)got;
            for (size_t i = 0; i < lim; i++) {
                unsigned char c = (unsigned char)lc((char)g_blk[i]);
                if (first[c]) mark_text(g_blk + i, (size_t)got - i);
            }
        }
    }
}

static void read_name(uint32_t rva, char* out, size_t cap) {
    out[0] = 0;
    int64_t off = rva_to_off(rva);
    if (off < 0) return;
    uint8_t tmp[96];
    int n = read_at(off, tmp, sizeof tmp - 1);
    if (n <= 0) return;
    tmp[n] = 0;
    size_t k = 0;
    while (k < (size_t)n && tmp[k] && k < cap - 1) { out[k] = (char)tmp[k]; k++; }
    out[k] = 0;
}

/* percorre a tabela de nomes (ILT/INT) de uma DLL; devolve quantas funcoes */
static int walk_thunks(uint32_t table_rva) {
    int64_t off = rva_to_off(table_rva);
    if (off < 0) return 0;
    int count = 0;
    for (; count < 30000; count++) {
        uint8_t e[8];
        if (read_at(off + (int64_t)count * 8, e, 8) != 8) break;
        uint64_t v = u64(e);
        if (v == 0) break;
        if (v >> 63) {
            rep("  #%u\n", (unsigned)(v & 0xFFFF));
        } else {
            char name[96];
            read_name((uint32_t)(v & 0x7FFFFFFF) + 2, name, sizeof name);
            rep("  %s\n", name[0] ? name : "?");
        }
    }
    return count;
}

void pe_analyze(const char* path, PeInfo* o, int report_fd) {
    memset(o, 0, sizeof *o);
    g_o = o;
    g_rep = report_fd;
    g_nsec = 0;
    g_fd = open(path, O_RDONLY);
    if (g_fd < 0) { fail("nao consegui abrir o arquivo"); return; }
    o->file_size = lseek(g_fd, 0, SEEK_END);

    uint8_t dos[64];
    if (read_at(0, dos, 64) != 64 || dos[0] != 'M' || dos[1] != 'Z') { fail("nao e um executavel Windows (sem MZ)"); close(g_fd); return; }
    int64_t nt = u32(dos + 0x3C);
    uint8_t h[24 + 240];
    int got = read_at(nt, h, sizeof h);
    if (got < 24 + 112 || memcmp(h, "PE\0\0", 4) != 0) { fail("cabecalho PE invalido"); close(g_fd); return; }
    o->machine = u16(h + 4);
    int nsec = u16(h + 6);
    int opt_size = u16(h + 20);
    const uint8_t* opt = h + 24;
    if (u16(opt) != 0x20B) { fail("nao e PE32+ (x64)"); close(g_fd); return; }
    o->entry_rva = u32(opt + 16);
    o->image_base = u64(opt + 24);
    o->size_of_image = u32(opt + 56);
    g_hdr_size = u32(opt + 60);
    o->subsystem = u16(opt + 68);
    uint32_t ndir = u32(opt + 108);
    if (ndir > 16) ndir = 16;
    uint32_t dir_va[16], dir_sz[16];
    memset(dir_va, 0, sizeof dir_va);
    memset(dir_sz, 0, sizeof dir_sz);
    for (uint32_t i = 0; i < ndir && 112 + (int)i * 8 + 8 <= got - 24; i++) {
        dir_va[i] = u32(opt + 112 + i * 8);
        dir_sz[i] = u32(opt + 116 + i * 8);
    }
    o->has_tls = dir_va[9] != 0;
    o->has_clr = dir_va[14] != 0;
    o->has_reloc = dir_va[5] != 0;

    if (nsec > 96) nsec = 96;
    for (int i = 0; i < nsec; i++) {
        uint8_t s[40];
        if (read_at(nt + 24 + opt_size + i * 40, s, 40) != 40) break;
        Sec* d = &g_sec[g_nsec++];
        memcpy(d->name, s, 8);
        d->name[8] = 0;
        d->vsize = u32(s + 8); d->va = u32(s + 12); d->rawsize = u32(s + 16); d->raw = u32(s + 20); d->flags = u32(s + 36);
    }
    o->sections = g_nsec;
    o->ok = 1;

    rep("== Cabecalho PE\n");
    rep("file_size=%lld machine=0x%04X subsystem=%u image_base=0x%llX size_of_image=0x%X entry_rva=0x%X\n",
        (long long)o->file_size, o->machine, o->subsystem, (unsigned long long)o->image_base, o->size_of_image, o->entry_rva);
    rep("tls=%d clr=%d reloc=%d (reloc dir %u bytes)\n", o->has_tls, o->has_clr, o->has_reloc, dir_sz[5]);
    rep("\n== Secoes (nome va vsize raw rawsize flags)\n");
    for (int i = 0; i < g_nsec; i++)
        rep("%-8s va=0x%08X vsize=0x%08X raw=0x%08X rawsize=0x%08X flags=0x%08X\n", g_sec[i].name, g_sec[i].va, g_sec[i].vsize,
            g_sec[i].raw, g_sec[i].rawsize, g_sec[i].flags);

    rep("\n== Imports\n");
    int64_t imp = dir_va[1] ? rva_to_off(dir_va[1]) : -1;
    for (int i = 0; imp >= 0 && i < 400; i++) {
        uint8_t d[20];
        if (read_at(imp + i * 20, d, 20) != 20) break;
        uint32_t name_rva = u32(d + 12);
        if (name_rva == 0) break;
        char dll[96];
        read_name(name_rva, dll, sizeof dll);
        mark_name(dll);
        rep("DLL %s\n", dll);
        uint32_t ilt = u32(d);
        int n = walk_thunks(ilt ? ilt : u32(d + 16));
        o->dlls++;
        o->funcs += n;
    }
    rep("\n== Imports com carga tardia (delay-load)\n");
    int64_t dly = dir_va[13] ? rva_to_off(dir_va[13]) : -1;
    for (int i = 0; dly >= 0 && i < 400; i++) {
        uint8_t d[32];
        if (read_at(dly + i * 32, d, 32) != 32) break;
        uint32_t name_rva = u32(d + 4);
        if (name_rva == 0) break;
        char dll[96];
        read_name(name_rva, dll, sizeof dll);
        mark_name(dll);
        rep("DLL %s (delay)\n", dll);
        int n = walk_thunks(u32(d + 16));
        o->delay_dlls++;
        o->delay_funcs += n;
    }
    scan_strings();
    rep("\n== Resumo\nDLLs=%d funcs=%d delay_dlls=%d delay_funcs=%d\n", o->dlls, o->funcs, o->delay_dlls, o->delay_funcs);
    rep("d3d12=%d d3d11=%d vulkan=%d opengl=%d dxc=%d steam=%d eos=%d oodle=%d bink=%d fmod=%d\n", o->hint_d3d12, o->hint_d3d11,
        o->hint_vulkan, o->hint_opengl, o->hint_dxc, o->hint_steam, o->hint_eos, o->hint_oodle, o->hint_bink, o->hint_fmod);
    close(g_fd);
}
