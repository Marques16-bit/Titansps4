/* Desenho das telas (so CPU, sem chamadas do PS4): da pra testar no PC com tools/preview.c. */
#pragma once
#include <stdint.h>

#include "gamescan.h"
#include "settings.h"
#include "sysinfo.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

#define UI_W 1920
#define UI_H 1080

enum { SCREEN_MAIN = 0, SCREEN_SETTINGS = 1, SCREEN_LOG = 2, SCREEN_GAME = 3 };
enum { MENU_START = 0, MENU_SETTINGS = 1, MENU_LOG = 2, MENU_EXIT = 3, MENU_COUNT = 4 };

typedef struct {
    int screen;                 /* SCREEN_* */
    int selected;               /* botao do menu principal */
    int set_sel;                /* linha em Settings: 0..S_COUNT-1 = opcao, S_COUNT = restaurar padroes */
    int log_scroll;             /* linhas acima do fim do log */
    int log_prev;               /* 1 = log da execucao anterior */
    int game_ready;             /* 1 = arquivos do jogo encontrados */
    const char* message;        /* aviso embaixo (pode ser NULL) */
    const char* log_text;       /* conteudo do log (NUL-terminado) */
    const Settings* settings;
    const SysInfo* sys;
    const GameReport* game;     /* resultado da analise (fase 1 do runtime) */
    int game_busy;              /* 1 = analisando */
} UiState;

/* fb: UI_W x UI_H pixels, bytes R,G,B,A na memoria (uint32 little-endian 0xAABBGGRR). */
void ui_draw(uint32_t* fb, const UiState* st);

int ui_log_visible_lines(void);
int ui_count_lines(const char* text);
