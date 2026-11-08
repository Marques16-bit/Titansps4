#include "settings.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char* const OFFON[] = {"Off", "On"};
static const char* const RES[] = {"1280x720", "1600x900", "1920x1080"};
static const char* const SCALE[] = {"50%", "60%", "70%", "75%", "80%", "90%", "100%"};
static const char* const FPS[] = {"30", "60", "Unlimited"};
static const char* const TEX[] = {"Low", "Medium", "High"};
static const char* const LEVEL[] = {"Off", "Low", "Medium", "High"};
static const char* const AA[] = {"Off", "FXAA", "TAA"};
static const char* const POS[] = {"Top left", "Top right"};
static const char* const VOL[] = {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"};
static const char* const LANG[] = {"English", "Portuguese", "Spanish", "French", "German", "Italian"};
static const char* const POOL[] = {"512 MB", "768 MB", "1024 MB", "1536 MB"};
static const char* const LOGLV[] = {"Error", "Info", "Debug"};

#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

const SettingDef k_settings[S_COUNT] = {
    [S_RESOLUTION]    = {"Video", "resolution", "Resolution", N(RES), RES, 1},
    [S_RENDER_SCALE]  = {"Video", "render_scale", "Render scale", N(SCALE), SCALE, 3},
    [S_FPS_LIMIT]     = {"Video", "fps_limit", "FPS limit", N(FPS), FPS, 0},
    [S_VSYNC]         = {"Video", "vsync", "VSync", N(OFFON), OFFON, 1},
    [S_TEXTURES]      = {"Video", "textures", "Textures", N(TEX), TEX, 1},
    [S_SHADOWS]       = {"Video", "shadows", "Shadows", N(LEVEL), LEVEL, 2},
    [S_BLOOM]         = {"Video", "bloom", "Bloom", N(OFFON), OFFON, 1},
    [S_SSR]           = {"Video", "ssr", "Screen-space reflections", N(LEVEL), LEVEL, 2},
    [S_AA]            = {"Video", "anti_aliasing", "Anti-aliasing", N(AA), AA, 1},
    [S_SHOW_FPS]      = {"Overlay", "show_fps", "Show FPS", N(OFFON), OFFON, 0},
    [S_SHOW_RAM]      = {"Overlay", "show_ram", "Show RAM", N(OFFON), OFFON, 0},
    [S_SHOW_TEMP]     = {"Overlay", "show_temperature", "Show temperature", N(OFFON), OFFON, 0},
    [S_SHOW_STORAGE]  = {"Overlay", "show_storage", "Show storage", N(OFFON), OFFON, 0},
    [S_OVERLAY_POS]   = {"Overlay", "overlay_position", "Overlay position", N(POS), POS, 1},
    [S_MASTER_VOL]    = {"Audio", "master_volume", "Master volume", N(VOL), VOL, 10},
    [S_MUSIC_VOL]     = {"Audio", "music_volume", "Music volume", N(VOL), VOL, 8},
    [S_SFX_VOL]       = {"Audio", "sfx_volume", "SFX volume", N(VOL), VOL, 10},
    [S_RUMBLE]        = {"Controls", "rumble", "Rumble", N(OFFON), OFFON, 1},
    [S_INVERT_Y]      = {"Controls", "invert_y", "Invert Y axis", N(OFFON), OFFON, 0},
    [S_LANGUAGE]      = {"System", "language", "Language", N(LANG), LANG, 0},
    [S_TEXTURE_POOL]  = {"System", "texture_pool", "Texture pool", N(POOL), POOL, 2},
    [S_SHADER_CACHE]  = {"System", "shader_cache", "Shader cache", N(OFFON), OFFON, 1},
    [S_LOG_LEVEL]     = {"System", "log_level", "Log level", N(LOGLV), LOGLV, 1},
};

void settings_defaults(Settings* s) {
    for (int i = 0; i < S_COUNT; i++) s->v[i] = k_settings[i].def;
}

const char* settings_value(const Settings* s, int id) {
    int v = s->v[id];
    if (v < 0 || v >= k_settings[id].count) v = k_settings[id].def;
    return k_settings[id].names[v];
}

/* E/S direta (open/read/write): o malloc padrao do OpenOrbis derruba o app neste console, e fopen aloca memoria. */
static char g_ini[4096];

static void trim(char* s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
}

static void apply_line(Settings* s, char* line) {
    trim(line);
    if (line[0] == '#' || line[0] == 0) return;
    char* eq = strchr(line, '=');
    if (!eq) return;
    *eq = 0;
    const char* key = line;
    const char* value = eq + 1;
    for (int i = 0; i < S_COUNT; i++) {
        if (strcmp(key, k_settings[i].key) != 0) continue;
        for (int j = 0; j < k_settings[i].count; j++)
            if (strcmp(value, k_settings[i].names[j]) == 0) s->v[i] = j;
    }
}

int settings_load(Settings* s, const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    int n = (int)read(fd, g_ini, sizeof g_ini - 1);
    close(fd);
    if (n < 0) return -1;
    g_ini[n] = 0;
    char* p = g_ini;
    while (*p) {
        char line[160];
        size_t k = 0;
        while (*p && *p != '\n') {
            if (k < sizeof line - 1) line[k++] = *p;
            p++;
        }
        if (*p == '\n') p++;
        line[k] = 0;
        apply_line(s, line);
    }
    return 0;
}

int settings_save(const Settings* s, const char* path) {
    int len = snprintf(g_ini, sizeof g_ini, "# Titans of the Tide (PS4) - opcoes. Pode editar: chave=valor.\n");
    const char* group = "";
    for (int i = 0; i < S_COUNT && len < (int)sizeof g_ini - 128; i++) {
        if (strcmp(group, k_settings[i].group) != 0) {
            group = k_settings[i].group;
            len += snprintf(g_ini + len, sizeof g_ini - len, "\n# %s\n", group);
        }
        len += snprintf(g_ini + len, sizeof g_ini - len, "%s=%s\n", k_settings[i].key, settings_value(s, i));
    }
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    int ok = write(fd, g_ini, (size_t)len) == len;
    close(fd);
    return ok ? 0 : -1;
}
