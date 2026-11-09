#include "ui.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "font8x16.h"
#include "paths.h"
#include "sintab.h"

#define RGB(r, g, b) (0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r))

static const uint32_t WHITE = RGB(255, 255, 255), YELLOW = RGB(250, 200, 40), DARK = RGB(12, 24, 52);
static const uint32_t PANEL = RGB(10, 28, 66), GREEN = RGB(120, 230, 140), ORANGE = RGB(255, 170, 70);
static const uint32_t GRAY = RGB(150, 160, 180), BLUE = RGB(30, 70, 140);

#define LOG_VISIBLE 46

static void rect(uint32_t* fb, int x, int y, int w, int h, uint32_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > UI_W) w = UI_W - x;
    if (y + h > UI_H) h = UI_H - y;
    for (int j = 0; j < h; j++) {
        uint32_t* p = fb + (size_t)(y + j) * UI_W + x;
        for (int i = 0; i < w; i++) p[i] = c;
    }
}

static void rect_alpha(uint32_t* fb, int x, int y, int w, int h, uint32_t c, int a) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > UI_W) w = UI_W - x;
    if (y + h > UI_H) h = UI_H - y;
    int sr = c & 255, sg = (c >> 8) & 255, sb = (c >> 16) & 255;
    for (int j = 0; j < h; j++) {
        uint32_t* p = fb + (size_t)(y + j) * UI_W + x;
        for (int i = 0; i < w; i++) {
            uint32_t d = p[i];
            int r = (sr * a + (int)(d & 255) * (255 - a)) / 255;
            int g = (sg * a + (int)((d >> 8) & 255) * (255 - a)) / 255;
            int b = (sb * a + (int)((d >> 16) & 255) * (255 - a)) / 255;
            p[i] = RGB(r, g, b);
        }
    }
}

static int text_width(const char* s, int scale) { return (int)strlen(s) * 8 * scale; }

static void text(uint32_t* fb, int x, int y, int scale, uint32_t c, const char* s) {
    for (; *s; s++, x += 8 * scale) {
        unsigned ch = (unsigned char)*s;
        if (x >= UI_W) break;
        if (ch < 32 || ch > 126) continue;
        const uint8_t* g = k_font8x16[ch - 32];
        for (int gy = 0; gy < 16; gy++)
            for (int gx = 0; gx < 8; gx++)
                if (g[gy] & (0x80 >> gx)) rect(fb, x + gx * scale, y + gy * scale, scale, scale, c);
    }
}

static void text_centered(uint32_t* fb, int cx, int y, int scale, uint32_t c, const char* s) {
    text(fb, cx - text_width(s, scale) / 2, y, scale, c, s);
}

static void text_right(uint32_t* fb, int rx, int y, int scale, uint32_t c, const char* s) {
    text(fb, rx - text_width(s, scale), y, scale, c, s);
}

static int abs_i(int v) { return v < 0 ? -v : v; }

static int isqrt_i(int n) {
    int r = 0;
    while ((r + 1) * (r + 1) <= n) r++;
    return r;
}

static void ring(uint32_t* fb, int cx, int cy, int r_out, int r_in, uint32_t c) {
    for (int dy = -r_out; dy <= r_out; dy++) {
        int xo = isqrt_i(r_out * r_out - dy * dy);
        int xi = (abs_i(dy) < r_in) ? isqrt_i(r_in * r_in - dy * dy) : -1;
        if (xi < 0) {
            rect(fb, cx - xo, cy + dy, 2 * xo + 1, 1, c);
        } else {
            rect(fb, cx - xo, cy + dy, xo - xi, 1, c);
            rect(fb, cx + xi + 1, cy + dy, xo - xi, 1, c);
        }
    }
}

static void arrow(uint32_t* fb, int cx, int ytip, int hw, int h, uint32_t c) {
    for (int i = 0; i < h; i++) {
        int w = hw * i / h;
        rect(fb, cx - w, ytip + i, 2 * w + 1, 1, c);
    }
}

static void emblem(uint32_t* fb, int cx, int cy) {
    ring(fb, cx, cy, 190, 172, YELLOW);
    rect(fb, cx - 8, cy - 110, 16, 270, YELLOW);   /* haste */
    rect(fb, cx - 66, cy - 30, 132, 14, YELLOW);   /* base dos dentes */
    rect(fb, cx - 66, cy - 100, 12, 80, YELLOW);
    rect(fb, cx + 54, cy - 100, 12, 80, YELLOW);
    arrow(fb, cx, cy - 160, 26, 56, YELLOW);
    arrow(fb, cx - 60, cy - 150, 20, 52, YELLOW);
    arrow(fb, cx + 60, cy - 150, 20, 52, YELLOW);
}

static void draw_background(uint32_t* fb) {
    for (int y = 0; y < UI_H; y++) {
        int t = y * 255 / (UI_H - 1);
        rect(fb, 0, y, UI_W, 1, RGB(10 + t * 10 / 255, 40 + t * 70 / 255, 90 + t * 70 / 255));
    }
    static const uint32_t wave_color[3] = {RGB(20, 90, 160), RGB(14, 66, 130), RGB(8, 44, 100)};
    for (int j = 0; j < 3; j++) {
        int base = 900 + j * 55, period = 700 + j * 260, phase = j * 70;
        for (int x = 0; x < UI_W; x++) {
            int y = base + 16 * k_sin[((x * 256 / period) + phase) & 255] / 127;
            rect(fb, x, y, 1, UI_H - y, wave_color[j]);
        }
    }
}

static void fmt_gb(char* out, size_t n, int64_t bytes) {
    if (bytes < 0) {
        snprintf(out, n, "n/a");
        return;
    }
    unsigned long long c = (unsigned long long)bytes * 100ull / (1ull << 30);
    snprintf(out, n, "%llu.%02llu GB", c / 100, c % 100);
}

static void fmt_temp(char* out, size_t n, int t) {
    if (t < 0) snprintf(out, n, "n/a");
    else snprintf(out, n, "%d C", t);
}

static void fmt_fps(char* out, size_t n, double fps) {
    int f10 = (int)(fps * 10.0 + 0.5);
    snprintf(out, n, "%d.%d", f10 / 10, f10 % 10);
}

static void header(uint32_t* fb, int h) { rect_alpha(fb, 0, 0, UI_W, h, DARK, 215); }

static void screen_main(uint32_t* fb, const UiState* st) {
    static const char* const labels[MENU_COUNT] = {"Start", "Settings", "Log", "Exit"};
    emblem(fb, 270, 640);
    header(fb, 150);
    text_centered(fb, UI_W / 2, 28, 3, WHITE, "SpongeBob SquarePants: Titans of the Tide");
    text_centered(fb, UI_W / 2, 98, 2, YELLOW, "PS4 port - test build v" APP_VERSION);

    rect(fb, 354, 176, 1212, 170, YELLOW);
    rect(fb, 360, 182, 1200, 158, PANEL);
    text(fb, 400, 202, 2, WHITE, "Status: Titans of the Tide not running");
    if (st->game_ready) text(fb, 400, 252, 2, GREEN, "Game files: found");
    else text(fb, 400, 252, 2, ORANGE, "Game files: not found");
    text(fb, 400, 302, 2, GRAY, "Folder: " GAME_DIR);

    for (int i = 0; i < MENU_COUNT; i++) {
        int x = 560, y = 390 + i * 128, w = 800, h = 100;
        int dim = (i == MENU_START && !st->game_ready);
        int sel = (st->selected == i);
        uint32_t fill = dim ? RGB(62, 74, 98) : (sel ? YELLOW : BLUE);
        uint32_t ink = dim ? RGB(120, 130, 150) : (sel ? DARK : WHITE);
        if (sel) rect(fb, x - 8, y - 8, w + 16, h + 16, WHITE);
        rect(fb, x, y, w, h, fill);
        text_centered(fb, x + w / 2, y + 26, 3, ink, labels[i]);
        if (sel) text(fb, x + 24, y + 26, 3, ink, ">");
    }
    text(fb, 1470, 470, 2, GRAY, "Up/Down  Move");
    text(fb, 1470, 510, 2, GRAY, "Cross    Select");
    text(fb, 1470, 550, 2, GRAY, "Circle   Back");
    if (st->message && st->message[0]) text_centered(fb, UI_W / 2, 896, 2, WHITE, st->message);
}

#define ROW_H 42
#define ROWS_VISIBLE 19

typedef struct { int item; const char* header; } Row;

static void screen_settings(uint32_t* fb, const UiState* st) {
    header(fb, 110);
    text(fb, 60, 30, 3, WHITE, "Settings");
    text_right(fb, UI_W - 60, 40, 2, YELLOW, "v" APP_VERSION);
    rect_alpha(fb, 40, 130, 1160, 830, PANEL, 225);
    rect_alpha(fb, 1230, 130, 650, 830, PANEL, 225);

    Row rows[S_COUNT + 12];
    int n = 0, sel_row = 0;
    const char* last = NULL;
    for (int i = 0; i < S_COUNT; i++) {
        if (!last || strcmp(last, k_settings[i].group) != 0) {
            rows[n].item = -1; rows[n].header = k_settings[i].group; n++;
            last = k_settings[i].group;
        }
        if (i == st->set_sel) sel_row = n;
        rows[n].item = i; rows[n].header = NULL; n++;
    }
    rows[n].item = -1; rows[n].header = "Maintenance"; n++;
    if (st->set_sel == S_COUNT) sel_row = n;
    rows[n].item = S_COUNT; rows[n].header = NULL; n++;

    int top = sel_row - ROWS_VISIBLE / 2;
    if (top > n - ROWS_VISIBLE) top = n - ROWS_VISIBLE;
    if (top < 0) top = 0;
    for (int r = 0; r < ROWS_VISIBLE && top + r < n; r++) {
        const Row* row = &rows[top + r];
        int y = 150 + r * ROW_H;
        if (row->item < 0) {
            text(fb, 70, y + 10, 2, YELLOW, row->header);
            continue;
        }
        int sel = (row->item == st->set_sel);
        if (sel) rect_alpha(fb, 56, y, 1128, ROW_H - 4, YELLOW, 70);
        if (row->item == S_COUNT) {
            text(fb, 90, y + 6, 2, ORANGE, "Restore defaults");
            continue;
        }
        text(fb, 90, y + 6, 2, WHITE, k_settings[row->item].label);
        char val[48];
        if (sel) snprintf(val, sizeof val, "< %s >", settings_value(st->settings, row->item));
        else snprintf(val, sizeof val, "%s", settings_value(st->settings, row->item));
        text_right(fb, 1160, y + 6, 2, sel ? YELLOW : RGB(200, 210, 225), val);
    }
    if (top > 0) text_right(fb, 1180, 134, 2, GRAY, "^");
    if (top + ROWS_VISIBLE < n) text_right(fb, 1180, 936, 2, GRAY, "v");

    char a[32], b[64];
    int y = 150;
    text(fb, 1260, y, 2, YELLOW, "Live status");
    y += 56;
    if (st->sys) {
        fmt_fps(a, sizeof a, st->sys->fps);
        snprintf(b, sizeof b, "FPS      %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_gb(a, sizeof a, st->sys->ram_free);
        snprintf(b, sizeof b, "RAM free %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_gb(a, sizeof a, st->sys->ram_total);
        snprintf(b, sizeof b, "RAM tot. %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_gb(a, sizeof a, st->sys->flex_free);
        snprintf(b, sizeof b, "Flex free %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_gb(a, sizeof a, st->sys->storage_free);
        snprintf(b, sizeof b, "Disk free %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_gb(a, sizeof a, st->sys->storage_total);
        snprintf(b, sizeof b, "Disk tot. %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_temp(a, sizeof a, st->sys->cpu_temp);
        snprintf(b, sizeof b, "CPU temp %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
        fmt_temp(a, sizeof a, st->sys->soc_temp);
        snprintf(b, sizeof b, "SoC temp %s", a);
        text(fb, 1260, y, 2, WHITE, b); y += 44;
    }
    text(fb, 1260, 800, 1, GRAY, "Settings file:");
    text(fb, 1260, 820, 1, WHITE, SETTINGS_FILE);
    text(fb, 1260, 850, 1, GRAY, "Game folder:");
    text(fb, 1260, 870, 1, WHITE, GAME_DIR);
    text(fb, 1260, 900, 1, GRAY, "n/a = leitura nao disponivel neste sistema");
    text_centered(fb, UI_W / 2, 1010, 2, GRAY, "Left/Right: change    Cross: next    Circle: back");
}


static void fmt_size(char* out, size_t n, int64_t bytes) {
    if (bytes < 0) { snprintf(out, n, "n/a"); return; }
    unsigned long long b = (unsigned long long)bytes;
    if (b < (1ull << 20)) { snprintf(out, n, "%llu KB", b >> 10); return; }
    if (b >= (1ull << 30)) snprintf(out, n, "%llu.%02llu GB", b >> 30, (b % (1ull << 30)) * 100 / (1ull << 30));
    else snprintf(out, n, "%llu.%llu MB", b >> 20, (b % (1ull << 20)) * 10 / (1ull << 20));
}

static const char* yes_no(int v) { return v ? "yes" : "no"; }

static void screen_game(uint32_t* fb, const UiState* st) {
    header(fb, 110);
    text(fb, 60, 30, 3, WHITE, "Game analysis");
    text_right(fb, UI_W - 60, 40, 2, YELLOW, "runtime phase 1");
    rect_alpha(fb, 40, 130, 1840, 840, PANEL, 225);
    const GameReport* g = st->game;
    if (st->game_busy || !g || !g->done) {
        text_centered(fb, UI_W / 2, 500, 3, WHITE, "Analyzing game files...");
        text_centered(fb, UI_W / 2, 570, 2, GRAY, "this can take a few seconds");
        return;
    }
    char a[64], b[64], line[256];
    int y = 150;
    const int step = 44;
    if (!g->dir_ok) {
        text(fb, 70, y, 2, ORANGE, "Folder listing unavailable. Create config/game.ini with: exe=relative/path.exe");
        y += step;
    }
    const char* exe = g->exe[0] ? g->exe : "(not found)";
    size_t len = strlen(exe);
    snprintf(line, sizeof line, "Executable: %s%s", len > 90 ? "..." : "", exe + (len > 90 ? len - 90 : 0));
    text(fb, 70, y, 2, g->exe[0] ? WHITE : ORANGE, line);
    y += step;
    text(fb, 70, y, 2, GRAY, "Source:");
    text(fb, 70 + 8 * 16, y, 2, GRAY, g->exe_source[0] ? g->exe_source : "-");
    y += step + 8;
    if (g->pe.ok) {
        const PeInfo* p = &g->pe;
        fmt_size(a, sizeof a, p->file_size);
        snprintf(line, sizeof line, "File size %s    Machine %s    Subsystem %s", a,
                 p->machine == 0x8664 ? "x64 (AMD64)" : "other", p->subsystem == 2 ? "Windows GUI" : (p->subsystem == 3 ? "Console" : "other"));
        text(fb, 70, y, 2, WHITE, line); y += step;
        snprintf(line, sizeof line, "Image base 0x%llX    Entry RVA 0x%X    Sections %d    TLS %s", (unsigned long long)p->image_base,
                 p->entry_rva, p->sections, yes_no(p->has_tls));
        text(fb, 70, y, 2, WHITE, line); y += step;
        snprintf(line, sizeof line, "Imports: %d DLLs, %d functions    Delay-load: %d DLLs, %d functions", p->dlls, p->funcs,
                 p->delay_dlls, p->delay_funcs);
        text(fb, 70, y, 2, YELLOW, line); y += step + 8;
        snprintf(line, sizeof line, "Graphics hints:  D3D12 %s   D3D11 %s   Vulkan %s   OpenGL %s   DXC %s", yes_no(p->hint_d3d12),
                 yes_no(p->hint_d3d11), yes_no(p->hint_vulkan), yes_no(p->hint_opengl), yes_no(p->hint_dxc));
        text(fb, 70, y, 2, WHITE, line); y += step;
        snprintf(line, sizeof line, "Middleware hints: Steam %s   EOS %s   Oodle %s   Bink %s   FMOD %s", yes_no(p->hint_steam),
                 yes_no(p->hint_eos), yes_no(p->hint_oodle), yes_no(p->hint_bink), yes_no(p->hint_fmod));
        text(fb, 70, y, 2, WHITE, line); y += step + 8;
    } else {
        snprintf(line, sizeof line, "Executable analysis failed: %s", g->pe.error);
        text(fb, 70, y, 2, ORANGE, line); y += step + 8;
    }
    fmt_size(a, sizeof a, g->total_bytes);
    fmt_size(b, sizeof b, g->pak_bytes);
    snprintf(line, sizeof line, "Game folder: %d files, %s    .pak/.utoc/.ucas: %d files, %s", g->files, a, g->pak_files, b);
    text(fb, 70, y, 2, WHITE, line); y += step;
    snprintf(line, sizeof line, ".exe files: %d    .dll files: %d", g->exe_files, g->dll_files);
    text(fb, 70, y, 2, WHITE, line); y += step + 8;
    text(fb, 70, y, 2, GRAY, "Full report (all imports):"); y += step;
    text(fb, 70, y, 2, WHITE, REPORT_FILE); y += step + 8;
    text(fb, 70, y, 2, ORANGE, "Runtime status: phase 1 (analysis only). The game cannot be executed yet.");
    text_centered(fb, UI_W / 2, 1010, 2, GRAY, "Cross: analyze again    Circle: back");
}

int ui_log_visible_lines(void) { return LOG_VISIBLE; }

int ui_count_lines(const char* t) {
    if (!t || !t[0]) return 0;
    int n = 0;
    for (const char* p = t; *p; p++) if (*p == '\n') n++;
    if (t[strlen(t) - 1] != '\n') n++;
    return n;
}

static void screen_log(uint32_t* fb, const UiState* st) {
    header(fb, 110);
    text(fb, 60, 30, 3, WHITE, "Log");
    text_right(fb, UI_W - 60, 40, 2, YELLOW, st->log_prev ? "previous run" : "current run");
    rect_alpha(fb, 40, 130, 1840, 840, RGB(4, 10, 24), 235);

    const char* p = st->log_text ? st->log_text : "";
    int total = ui_count_lines(p);
    int first = total - LOG_VISIBLE - st->log_scroll;
    if (first < 0) first = 0;
    for (int i = 0; i < first && *p; i++) {
        const char* nl = strchr(p, '\n');
        if (!nl) { p += strlen(p); break; }
        p = nl + 1;
    }
    for (int i = 0; i < LOG_VISIBLE && *p; i++) {
        char line[232];
        size_t k = 0;
        while (*p && *p != '\n') {
            if (k < sizeof line - 1) line[k++] = *p;
            p++;
        }
        if (*p == '\n') p++;
        line[k] = 0;
        int bad = strstr(line, "CRASH") || strstr(line, "failed");
        text(fb, 60, 142 + i * 18, 1, bad ? ORANGE : RGB(200, 210, 225), line);
    }
    text_centered(fb, UI_W / 2, 1010, 2, GRAY, "Up/Down: scroll    Triangle: current/previous    Circle: back");
}

static void overlay(uint32_t* fb, const UiState* st) {
    if (!st->settings || !st->sys) return;
    char lines[4][48], a[32];
    int n = 0;
    if (st->settings->v[S_SHOW_FPS]) {
        fmt_fps(a, sizeof a, st->sys->fps);
        snprintf(lines[n++], 48, "FPS %s", a);
    }
    if (st->settings->v[S_SHOW_RAM]) {
        char t[32];
        fmt_gb(a, sizeof a, st->sys->ram_free);
        fmt_gb(t, sizeof t, st->sys->ram_total);
        snprintf(lines[n++], 48, "RAM %s / %s", a, t);
    }
    if (st->settings->v[S_SHOW_TEMP]) {
        fmt_temp(a, sizeof a, st->sys->cpu_temp);
        snprintf(lines[n++], 48, "CPU %s", a);
    }
    if (st->settings->v[S_SHOW_STORAGE]) {
        fmt_gb(a, sizeof a, st->sys->storage_free);
        snprintf(lines[n++], 48, "DISK %s free", a);
    }
    if (!n) return;
    int w = 0;
    for (int i = 0; i < n; i++) if ((int)strlen(lines[i]) * 16 > w) w = (int)strlen(lines[i]) * 16;
    w += 28;
    int h = n * 36 + 14;
    int x = st->settings->v[S_OVERLAY_POS] == 1 ? UI_W - w - 24 : 24;
    rect_alpha(fb, x, 16, w, h, RGB(0, 0, 0), 175);
    for (int i = 0; i < n; i++) text(fb, x + 14, 24 + i * 36, 2, GREEN, lines[i]);
}

void ui_draw(uint32_t* fb, const UiState* st) {
    static uint32_t bg[UI_W * UI_H];
    static int bg_ready;
    if (!bg_ready) {
        draw_background(bg);
        bg_ready = 1;
    }
    memcpy(fb, bg, sizeof bg);
    if (st->screen == SCREEN_SETTINGS) screen_settings(fb, st);
    else if (st->screen == SCREEN_LOG) screen_log(fb, st);
    else if (st->screen == SCREEN_GAME) screen_game(fb, st);
    else screen_main(fb, st);
    overlay(fb, st);
}
