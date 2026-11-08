/* Leituras do sistema para o overlay e para a tela Settings. -1 = indisponivel. */
#pragma once
#include <stdint.h>

typedef struct {
    double fps;
    int64_t ram_total, ram_free;       /* memoria direta (bytes); free = maior bloco livre */
    int64_t flex_free;                 /* memoria flexivel livre (bytes) */
    int64_t storage_total, storage_free;
    int cpu_temp, soc_temp;            /* graus C */
} SysInfo;

void sysinfo_init(SysInfo* s);
void sysinfo_update(SysInfo* s);       /* chame ~1x por segundo */
