/* Titans of the Tide (PS4): menu de teste. Tela por VideoOut (CPU) e controle por ScePad.
 * Partes (VideoOut em memoria direta write-combined, ScePad) seguem o runtime do SMW (iHaiDeeZ/SMW, MIT). */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>

#include "ui.h"

#define APP_DIR  "/data/titans-of-the-tide"
#define GAME_DIR APP_DIR "/game"
#define GAME_MARK GAME_DIR "/Engine" /* todo jogo UE empacotado tem a pasta Engine na raiz */
#define LOG_FILE "/data/titans-of-the-tide.log"

/* Nomes proprios com o simbolo do sistema: evita conflito com as assinaturas dos headers. */
extern int32_t tp_alloc_direct(off_t start, off_t end, size_t len, size_t align, int32_t type, off_t* phys)
    __asm__("sceKernelAllocateDirectMemory");
extern int32_t tp_map_direct(void** addr, size_t len, int32_t prot, int32_t flags, off_t phys, size_t align)
    __asm__("sceKernelMapDirectMemory");
extern size_t tp_direct_size(void) __asm__("sceKernelGetDirectMemorySize");

#define BUFFER_SIZE (8u << 20)  /* 1920x1080x4 arredondado para paginas de 2 MiB */
#define WC_GARLIC 3             /* memoria write-combined que o display le */
#define PROT_CPU_GPU 0x33       /* CPU e GPU leem/escrevem */

static uint32_t g_frame[UI_W * UI_H]; /* o quadro e montado aqui (RAM comum) e copiado de uma vez */

static void logf_(const char* fmt, ...) {
    char line[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    printf("titans: %s\n", line);
    FILE* f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
}

static int exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int main(void) {
    logf_("start");

    sceUserServiceInitialize(NULL);
    scePadInit();
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0) user = -1;
    int pad = scePadOpen(user, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
    logf_("pad handle %d (user %08x)", pad, (unsigned)user);

    int video = sceVideoOutOpen(0xFF, 0, 0, NULL);
    if (video < 0) {
        logf_("sceVideoOutOpen failed: %x", (unsigned)video);
        return 1;
    }
    off_t phys = 0;
    void* base = NULL;
    if (tp_alloc_direct(0, (off_t)tp_direct_size(), 2 * BUFFER_SIZE, 2u << 20, WC_GARLIC, &phys) != 0 ||
        tp_map_direct(&base, 2 * BUFFER_SIZE, PROT_CPU_GPU, 0, phys, 2u << 20) != 0) {
        logf_("no direct memory for the frame buffers");
        return 1;
    }
    uint8_t* buffers[2] = {(uint8_t*)base, (uint8_t*)base + BUFFER_SIZE};
    void* addresses[2] = {buffers[0], buffers[1]};
    OrbisVideoOutBufferAttribute attribute;
    sceVideoOutSetBufferAttribute(&attribute, ORBIS_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB,
                                  ORBIS_VIDEO_OUT_TILING_MODE_LINEAR, 0, UI_W, UI_H, UI_W);
    int reg = sceVideoOutRegisterBuffers(video, 0, addresses, 2, &attribute);
    if (reg < 0) {
        logf_("sceVideoOutRegisterBuffers failed: %x", (unsigned)reg);
        return 1;
    }
    logf_("video ready");

    UiState st;
    memset(&st, 0, sizeof st);
    st.game_ready = exists(GAME_MARK);
    uint32_t prev = 0;
    int k = 0, quit = 0;
    for (unsigned frame = 0; !quit; frame++) {
        if (pad < 0 && frame % 30 == 0) pad = scePadOpen(user, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
        if (frame % 30 == 0) st.game_ready = exists(GAME_MARK);

        OrbisPadData d;
        memset(&d, 0, sizeof d);
        uint32_t buttons = 0;
        if (pad >= 0 && scePadReadState(pad, &d) == 0 && d.connected) buttons = d.buttons;
        uint32_t pressed = buttons & ~prev;
        prev = buttons;

        if (pressed & ORBIS_PAD_BUTTON_UP) st.selected = (st.selected + UI_BTN_COUNT - 1) % UI_BTN_COUNT;
        if (pressed & ORBIS_PAD_BUTTON_DOWN) st.selected = (st.selected + 1) % UI_BTN_COUNT;
        if (pressed & ORBIS_PAD_BUTTON_CROSS) {
            if (st.selected == UI_BTN_START) {
                if (!st.game_ready) {
                    st.message = "Jogo nao encontrado: use Criar caminho e copie os arquivos";
                } else {
                    st.message = "Runtime do jogo ainda nao implementado (fase 3)";
                    logf_("start pressed: runtime not implemented");
                }
            } else if (st.selected == UI_BTN_PATH) {
                int had = exists(GAME_DIR);
                mkdir(APP_DIR, 0777);
                mkdir(GAME_DIR, 0777);
                if (!exists(GAME_DIR))
                    st.message = "Falha ao criar a pasta (veja /data/titans-of-the-tide.log)";
                else if (had)
                    st.message = "A pasta ja existe: " GAME_DIR;
                else
                    st.message = "Pasta criada: " GAME_DIR;
                logf_("create path: %s", st.message);
            } else {
                quit = 1;
            }
        }

        ui_draw(g_frame, &st);
        memcpy(buffers[k], g_frame, sizeof g_frame);
        __atomic_thread_fence(__ATOMIC_SEQ_CST); /* stores write-combined saem antes do flip */
        sceVideoOutSubmitFlip(video, k, ORBIS_VIDEO_OUT_FLIP_VSYNC, 0);
        if (frame == 0) sceSystemServiceHideSplashScreen();
        k ^= 1;
        sceKernelUsleep(33000); /* o outro buffer ja saiu da tela */
    }
    logf_("exit");
    return 0;
}
