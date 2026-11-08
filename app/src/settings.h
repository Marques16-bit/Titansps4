/* Opcoes do menu Settings. Salvas em /data/Titans/config/settings.ini como chave=valor (o nome do valor). */
#pragma once

enum {
    S_RESOLUTION, S_RENDER_SCALE, S_FPS_LIMIT, S_VSYNC, S_TEXTURES, S_SHADOWS, S_BLOOM, S_SSR, S_AA,
    S_SHOW_FPS, S_SHOW_RAM, S_SHOW_TEMP, S_SHOW_STORAGE, S_OVERLAY_POS,
    S_MASTER_VOL, S_MUSIC_VOL, S_SFX_VOL,
    S_RUMBLE, S_INVERT_Y,
    S_LANGUAGE, S_TEXTURE_POOL, S_SHADER_CACHE, S_LOG_LEVEL,
    S_COUNT
};

typedef struct {
    const char* group;
    const char* key;
    const char* label;
    int count;
    const char* const* names;
    int def;
} SettingDef;

extern const SettingDef k_settings[S_COUNT];

typedef struct { int v[S_COUNT]; } Settings;

void settings_defaults(Settings* s);
int settings_load(Settings* s, const char* path);       /* 0 = ok, -1 = sem arquivo */
int settings_save(const Settings* s, const char* path); /* 0 = ok, -1 = erro */
const char* settings_value(const Settings* s, int id);
