/* Gera um PPM do menu no PC: gcc -O2 -Iapp/src tools/preview.c app/src/ui.c -o preview && ./preview out.ppm [ready] [sel=N] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

static uint32_t fb[UI_W * UI_H];

int main(int argc, char** argv) {
    UiState st = {0, 0, "Copie os arquivos do jogo para /data/titans-of-the-tide/game"};
    const char* out = argc > 1 ? argv[1] : "preview.ppm";
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "ready")) { st.game_ready = 1; st.message = "Runtime do jogo ainda nao implementado (fase 3)"; }
        if (!strncmp(argv[i], "sel=", 4)) st.selected = atoi(argv[i] + 4);
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
