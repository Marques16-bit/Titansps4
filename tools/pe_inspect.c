/* PC: gcc -O2 -DTITANS_HOST -Iapp/src tools/pe_inspect.c app/src/gamescan.c app/src/pe.c app/src/dirlist.c -o pe_inspect
 *     ./pe_inspect <pasta-do-jogo> [game.ini] [relatorio.txt]    (mesma analise que o botao Start faz no PS4) */
#include <stdio.h>

#include "gamescan.h"

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "uso: %s <pasta-do-jogo> [game.ini] [relatorio.txt]\n", argv[0]); return 1; }
    static GameReport r;
    gamescan_run(&r, argv[1], argc > 2 ? argv[2] : "/nonexistent.ini", argc > 3 ? argv[3] : "/tmp/pe_report.txt");
    printf("exe=%s (%s)\ndir_ok=%d arquivos=%d bytes=%lld pak=%d (%lld) exe=%d dll=%d\n", r.exe, r.exe_source, r.dir_ok, r.files,
           (long long)r.total_bytes, r.pak_files, (long long)r.pak_bytes, r.exe_files, r.dll_files);
    if (r.pe.ok)
        printf("PE ok: machine=0x%X subsystem=%u base=0x%llX entry=0x%X sections=%d dlls=%d funcs=%d delay=%d/%d\n"
               "hints: d3d12=%d d3d11=%d vulkan=%d opengl=%d dxc=%d steam=%d eos=%d oodle=%d bink=%d fmod=%d\n",
               r.pe.machine, r.pe.subsystem, (unsigned long long)r.pe.image_base, r.pe.entry_rva, r.pe.sections, r.pe.dlls,
               r.pe.funcs, r.pe.delay_dlls, r.pe.delay_funcs, r.pe.hint_d3d12, r.pe.hint_d3d11, r.pe.hint_vulkan, r.pe.hint_opengl,
               r.pe.hint_dxc, r.pe.hint_steam, r.pe.hint_eos, r.pe.hint_oodle, r.pe.hint_bink, r.pe.hint_fmod);
    else printf("PE falhou: %s\n", r.pe.error);
    return r.pe.ok ? 0 : 2;
}
