/* PC: gcc -O2 -Iapp/src tools/preview.c app/src/ui.c -o preview
 *     ./preview out.ppm [main|settings|log] [ready] [sel=N] [set=N] [overlay]      |     ./preview selftest */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "../app/src/settings.c" /* junto aqui para compilar so com ui.c */

static uint32_t fb[UI_W * UI_H];

static int selftest(void) {
    Settings a, b;
    settings_defaults(&a);
    a.v[S_FPS_LIMIT] = 1; a.v[S_SHOW_FPS] = 1; a.v[S_LANGUAGE] = 2;
    if (settings_save(&a, "/tmp/titans_test.ini") != 0) return 1;
    settings_defaults(&b);
    if (settings_load(&b, "/tmp/titans_test.ini") != 0) return 2;
    for (int i = 0; i < S_COUNT; i++) if (a.v[i] != b.v[i]) { printf("diff em %s\n", k_settings[i].key); return 3; }
    if (settings_load(&b, "/tmp/nao_existe.ini") != -1) return 4;
    printf("selftest ok (%d opcoes)\n", S_COUNT);
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && !strcmp(argv[1], "selftest")) return selftest();
    static Settings set;
    static SysInfo sys = {30.0, 4800000000LL, 3600000000LL, 900000000LL, 457000000000LL, 212000000000LL, 61, 58};
    settings_defaults(&set);
    UiState st;
    memset(&st, 0, sizeof st);
    st.settings = &set;
    st.sys = &sys;
    st.message = "Jogo nao encontrado: copie os arquivos para /data/Titans/game";
    static char log[4096];
    snprintf(log, sizeof log, "start v%s\nmodule SystemService: 0\nmodule UserService: 0\nmodule VideoOut: 0\nmodule Pad: 0\n"
             "pad handle 0 (user 10000000)\nvideo ready\nCRASH signal 11 at 0x0\nsceVideoOutOpen failed: 80020001\n", APP_VERSION);
    st.log_text = log;
    const char* out = argc > 1 ? argv[1] : "preview.ppm";
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "settings")) st.screen = SCREEN_SETTINGS;
        if (!strcmp(argv[i], "log")) st.screen = SCREEN_LOG;
        if (!strcmp(argv[i], "ready")) { st.game_ready = 1; st.message = "Runtime do jogo ainda nao implementado (fase 3)"; }
        if (!strncmp(argv[i], "sel=", 4)) st.selected = atoi(argv[i] + 4);
        if (!strncmp(argv[i], "set=", 4)) st.set_sel = atoi(argv[i] + 4);
        if (!strcmp(argv[i], "overlay")) { set.v[S_SHOW_FPS] = set.v[S_SHOW_RAM] = set.v[S_SHOW_TEMP] = set.v[S_SHOW_STORAGE] = 1; }
    }
    ui_draw(fb, &st);
    FILE* f = fopen(out, "wb");
    if (!f) return 1;
    fprintf(f, "P6\n%d %d\n255\n", UI_W, UI_H);
    for (int i = 0; i < UI_W * UI_H; i++) {
        unsigned char rgb[3] = {(unsigned char)(fb[i] & 255), (unsigned char)((fb[i] >> 8) & 255), (unsigned char)((fb[i] >> 16) & 255)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    return 0;
}
