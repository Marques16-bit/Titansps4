#include "ui.h"

#include <stddef.h>
#include <string.h>

#include "font8x16.h"

#define RGB(r, g, b) (0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r))

#define GAME_DIR_TEXT "/data/titans-of-the-tide/game"

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

static int text_width(const char* s, int scale) { return (int)strlen(s) * 8 * scale; }

static void text(uint32_t* fb, int x, int y, int scale, uint32_t c, const char* s) {
    for (; *s; s++, x += 8 * scale) {
        unsigned ch = (unsigned char)*s;
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

void ui_draw(uint32_t* fb, const UiState* st) {
    static const char* const labels[UI_BTN_COUNT] = {"Iniciar", "Criar caminho", "Exit"};
    const uint32_t white = RGB(255, 255, 255), yellow = RGB(250, 200, 40), dark = RGB(12, 24, 52);
    const uint32_t panel = RGB(10, 28, 66), green = RGB(120, 230, 140), orange = RGB(255, 170, 70);
    const uint32_t gray = RGB(150, 160, 180);

    /* fundo: degrade vertical azul */
    for (int y = 0; y < UI_H; y++) {
        int t = y * 255 / (UI_H - 1);
        rect(fb, 0, y, UI_W, 1, RGB(10 + t * 10 / 255, 40 + t * 70 / 255, 90 + t * 70 / 255));
    }

    /* cabecalho */
    rect(fb, 0, 0, UI_W, 150, RGB(8, 22, 56));
    text_centered(fb, UI_W / 2, 28, 3, white, "SpongeBob SquarePants: Titans of the Tide");
    text_centered(fb, UI_W / 2, 98, 2, yellow, "PS4 port - early test build");

    /* painel de status */
    rect(fb, 354, 204, 1212, 182, yellow);
    rect(fb, 360, 210, 1200, 170, panel);
    text(fb, 400, 232, 2, white, "Status: Titans of the Tide not running");
    if (st->game_ready)
        text(fb, 400, 282, 2, green, "Game files: found");
    else
        text(fb, 400, 282, 2, orange, "Game files: not found");
    text(fb, 400, 332, 2, gray, "Folder: " GAME_DIR_TEXT);

    /* botoes */
    for (int i = 0; i < UI_BTN_COUNT; i++) {
        int x = 560, y = 450 + i * 150, w = 800, h = 110;
        int disabled = (i == UI_BTN_START && !st->game_ready);
        int selected = (st->selected == i);
        uint32_t fill = disabled ? RGB(62, 74, 98) : (selected ? yellow : RGB(30, 70, 140));
        uint32_t ink = disabled ? RGB(120, 130, 150) : (selected ? dark : white);
        if (selected) rect(fb, x - 8, y - 8, w + 16, h + 16, white);
        rect(fb, x, y, w, h, fill);
        text_centered(fb, x + w / 2, y + 31, 3, ink, labels[i]);
        if (selected) text(fb, x + 24, y + 31, 3, ink, ">");
    }

    /* aviso e rodape */
    if (st->message && st->message[0]) text_centered(fb, UI_W / 2, 912, 2, white, st->message);
    text_centered(fb, UI_W / 2, 1010, 2, gray, "Up/Down: move    Cross: select");
}
