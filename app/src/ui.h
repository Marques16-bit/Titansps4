/* Desenho do menu (so CPU, sem chamadas do PS4): da pra testar no PC com tools/preview.c. */
#pragma once
#include <stdint.h>

#define UI_W 1920
#define UI_H 1080

enum { UI_BTN_START = 0, UI_BTN_PATH = 1, UI_BTN_EXIT = 2, UI_BTN_COUNT = 3 };

typedef struct {
    int selected;           /* botao em foco: UI_BTN_* */
    int game_ready;         /* 1 = arquivos do jogo encontrados */
    const char* message;    /* linha de aviso embaixo (pode ser NULL) */
} UiState;

/* fb: UI_W x UI_H pixels, bytes R,G,B,A na memoria (uint32 little-endian 0xAABBGGRR). */
void ui_draw(uint32_t* fb, const UiState* st);
