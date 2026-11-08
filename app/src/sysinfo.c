/* So existe no build do PS4. Cada API opcional so entra se a toolchain exporta o simbolo (build-pkg.sh confere). */
#include "sysinfo.h"

#include <stddef.h>
#include <string.h>

#include "paths.h"

/* Nomes proprios com o simbolo do sistema: evita conflito com as assinaturas dos headers. */
extern size_t ti_direct_size(void) __asm__("sceKernelGetDirectMemorySize");
extern int ti_avail_direct(int64_t start, int64_t end, size_t align, int64_t* phys, size_t* size)
    __asm__("sceKernelAvailableDirectMemorySize");
extern int ti_avail_flex(size_t* size) __asm__("sceKernelAvailableFlexibleMemorySize");
#ifdef HAVE_KERNEL_CPU_TEMP
extern int ti_cpu_temp(int* t) __asm__("sceKernelGetCpuTemperature");
#endif
#ifdef HAVE_KERNEL_SOC_TEMP
extern int ti_soc_temp(int sensor, int* t) __asm__("sceKernelGetSocSensorTemperature");
#endif
#ifdef HAVE_KERNEL_STATFS
extern int ti_statfs(const char* path, void* buf) __asm__("statfs");
#endif

void sysinfo_init(SysInfo* s) {
    memset(s, 0, sizeof *s);
    s->ram_total = s->ram_free = s->flex_free = -1;
    s->storage_total = s->storage_free = -1;
    s->cpu_temp = s->soc_temp = -1;
}

void sysinfo_update(SysInfo* s) {
    size_t total = ti_direct_size();
    s->ram_total = (int64_t)total;
    int64_t phys = 0;
    size_t largest = 0;
    s->ram_free = ti_avail_direct(0, (int64_t)total, 2u << 20, &phys, &largest) == 0 ? (int64_t)largest : -1;
    size_t flex = 0;
    s->flex_free = ti_avail_flex(&flex) == 0 ? (int64_t)flex : -1;

#ifdef HAVE_KERNEL_CPU_TEMP
    int t = 0;
    s->cpu_temp = ti_cpu_temp(&t) == 0 && t > 0 && t < 150 ? t : -1;
#endif
#ifdef HAVE_KERNEL_SOC_TEMP
    int u = 0;
    s->soc_temp = ti_soc_temp(0, &u) == 0 && u > 0 && u < 150 ? u : -1;
#endif
#ifdef HAVE_KERNEL_STATFS
    /* struct statfs do FreeBSD 9: version(4) type(4) flags(8) bsize(8) iosize(8) blocks(8) bfree(8) bavail(8) ... */
    union { unsigned char raw[1024]; struct { uint32_t ver, type; uint64_t flags, bsize, iosize, blocks, bfree; int64_t bavail; } f; } u2;
    memset(&u2, 0, sizeof u2);
    if (ti_statfs("/data", &u2) == 0 && u2.f.bsize > 0 && u2.f.blocks > 0) {
        s->storage_total = (int64_t)(u2.f.blocks * u2.f.bsize);
        s->storage_free = (int64_t)((u2.f.bavail > 0 ? (uint64_t)u2.f.bavail : 0) * u2.f.bsize);
    }
#endif
}
