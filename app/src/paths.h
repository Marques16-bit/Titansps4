/* Estrutura de pastas do app no PS4. Tudo fica em /data/Titans. */
#pragma once

#ifndef TITANS_ROOT
#define TITANS_ROOT "/data/Titans"
#endif

#define GAME_DIR      TITANS_ROOT "/game"              /* arquivos do jogo (versao de PC) */
#define CONFIG_DIR    TITANS_ROOT "/config"            /* opcoes que a pessoa muda no menu */
#define SETTINGS_FILE CONFIG_DIR "/settings.ini"
#define LOGS_DIR      TITANS_ROOT "/logs"
#define LOG_FILE      LOGS_DIR "/app.log"              /* execucao atual */
#define LOG_PREV      LOGS_DIR "/app.prev.log"         /* execucao anterior */
#define SAVES_DIR     TITANS_ROOT "/saves"             /* saves do jogo */
#define CACHE_DIR     TITANS_ROOT "/cache"
#define SHADER_DIR    CACHE_DIR "/shaders"             /* cache de shaders */
#define GAME_MARK     GAME_DIR "/Engine"               /* todo jogo UE empacotado tem Engine/ na raiz */
