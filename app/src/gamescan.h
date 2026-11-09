/* Fase 1 do runtime: acha o executavel do jogo, analisa e grava o relatorio. */
#pragma once
#include <stdint.h>

#include "pe.h"

typedef struct {
    int done;
    int dir_ok;                 /* a listagem de pastas funcionou */
    char exe[240];              /* caminho completo do .exe analisado ("" = nao achou) */
    char exe_source[24];        /* "game.ini" ou "auto" */
    PeInfo pe;
    int files;
    int64_t total_bytes;
    int pak_files;
    int64_t pak_bytes;          /* .pak/.utoc/.ucas */
    int exe_files, dll_files;
} GameReport;

void gamescan_run(GameReport* r, const char* game_dir, const char* ini_path, const char* report_path);
