/* Analise de executavel Windows (PE32+ x64) sem malloc: so open/lseek/read. */
#pragma once
#include <stdint.h>

typedef struct {
    int ok;
    char error[96];
    uint16_t machine, subsystem;
    uint64_t image_base;
    uint32_t size_of_image, entry_rva;
    int sections;
    int dlls, funcs, delay_dlls, delay_funcs;
    int has_tls, has_clr, has_reloc;
    /* dicas: achadas nos imports ou em strings do .rdata */
    int hint_d3d12, hint_d3d11, hint_vulkan, hint_opengl, hint_dxc;
    int hint_steam, hint_eos, hint_oodle, hint_bink, hint_fmod;
    int64_t file_size;
} PeInfo;

/* report_fd >= 0: grava no arquivo o cabecalho, as secoes e todos os imports. */
void pe_analyze(const char* path, PeInfo* out, int report_fd);
