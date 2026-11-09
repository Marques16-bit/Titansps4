/* Titans of the Tide (PS4): menu Start / Settings / Log / Exit.
 * Video: buffers em memoria direta write-combined + sceVideoOut. Controle: ScePad.
 * IMPORTANTE: os modulos do sistema (UserService, VideoOut, Pad...) precisam ser carregados antes do uso; chamar uma
 * biblioteca sem o modulo carregado fecha o app sem nenhuma mensagem. */
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>

#include "gamescan.h"
#include "paths.h"
#include "settings.h"
#include "sysinfo.h"
#include "ui.h"

/* Nomes proprios com o simbolo do sistema: evita conflito com as assinaturas dos headers. */
extern int32_t tp_alloc_direct(off_t start, off_t end, size_t len, size_t align, int32_t type, off_t* phys)
    __asm__("sceKernelAllocateDirectMemory");
extern int32_t tp_map_direct(void** addr, size_t len, int32_t prot, int32_t flags, off_t phys, size_t align)
    __asm__("sceKernelMapDirectMemory");
extern size_t tp_direct_size(void) __asm__("sceKernelGetDirectMemorySize");
extern int32_t tp_load_module(uint32_t id) __asm__("sceSysmoduleLoadModuleInternal");

#ifdef HAVE_KERNEL_NOTIFY
/* Aviso na tela do PS4 (toast): mostra ate onde a inicializacao chegou, mesmo se o app morrer logo depois. */
extern int ti_notify(int type, void* request, size_t size, int blocking) __asm__("sceKernelSendNotificationRequest");
static struct { char pad[45]; char message[3075]; } g_notify; /* 3120 bytes, formato esperado pelo sistema */
static void toast(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    memset(&g_notify, 0, sizeof g_notify);
    vsnprintf(g_notify.message, 200, fmt, ap);
    va_end(ap);
    ti_notify(0, &g_notify, sizeof g_notify, 0);
}
#else
static void toast(const char* fmt, ...) { (void)fmt; }
#endif

#define FIRST_BUFFER 12         /* indice do primeiro buffer registrado no VideoOut */
#define BUFFER_SIZE (8u << 20)  /* 1920x1080x4 arredondado para paginas de 2 MiB */
#define WC_GARLIC 3             /* memoria write-combined que o display le */
#define PROT_CPU_GPU 0x33       /* CPU e GPU leem/escrevem */
#define LOG_TAIL 24000

static uint32_t g_frame[UI_W * UI_H]; /* o quadro e montado aqui (RAM comum) e copiado de uma vez */
static char g_log[LOG_TAIL + 1];

/* Sem stdio de arquivos (fopen/fprintf): o malloc padrao do OpenOrbis falha neste console e derruba o app na primeira
 * alocacao. open/write nao alocam memoria. */
static void logf_(const char* fmt, ...) {
    char line[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line - 1, fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if (n > (int)sizeof line - 2) n = (int)sizeof line - 2;
    line[n] = '\n';
    line[n + 1] = 0;
    printf("titans: %s", line);
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd >= 0) {
        write(fd, line, (size_t)n + 1);
        close(fd);
    }
}

static int exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static void make_folders(void) {
    static const char* const dirs[] = {TITANS_ROOT, GAME_DIR, CONFIG_DIR, LOGS_DIR, SAVES_DIR, CACHE_DIR, SHADER_DIR};
    for (unsigned i = 0; i < sizeof dirs / sizeof dirs[0]; i++) mkdir(dirs[i], 0777);
}

/* signal() padrao: o signal.h do OpenOrbis tem o macro sa_sigaction com o nome de campo errado e nao compila. */
static void crash_handler(int sig) {
    logf_("CRASH signal %d", sig);
    toast("Titans: CRASH sinal %d", sig);
    signal(sig, SIG_DFL);
    raise(sig);
}

static void load_system_modules(void) {
    static const struct { const char* name; uint32_t id; } mods[] = {
        {"SystemService", 0x80000010}, {"UserService", 0x80000011}, {"VideoOut", 0x80000022}, {"Pad", 0x80000024},
    };
    for (unsigned i = 0; i < sizeof mods / sizeof mods[0]; i++) {
        int32_t r = tp_load_module(mods[i].id);
        logf_("module %s: %x", mods[i].name, (unsigned)r);
    }
}

static void load_log(int prev) {
    const char* path = prev ? LOG_PREV : LOG_FILE;
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        snprintf(g_log, sizeof g_log, "(sem log em %s)\n", path);
        return;
    }
    off_t size = lseek(fd, 0, SEEK_END);
    off_t start = size > LOG_TAIL ? size - LOG_TAIL : 0;
    lseek(fd, start, SEEK_SET);
    ssize_t n = read(fd, g_log, LOG_TAIL);
    close(fd);
    if (n < 0) n = 0;
    g_log[n] = 0;
    if (start > 0) {
        char* nl = strchr(g_log, '\n');
        if (nl) memmove(g_log, nl + 1, strlen(nl + 1) + 1);
    }
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int main(void) {
    /* pastas e log primeiro: se algo falhar depois, o motivo fica em /data/Titans/logs/app.log */
    mkdir(TITANS_ROOT, 0777);
    mkdir(LOGS_DIR, 0777);
    remove(LOG_PREV);
    rename(LOG_FILE, LOG_PREV);
    logf_("start v" APP_VERSION);
    signal(SIGSEGV, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGILL, crash_handler);
    make_folders();

    static Settings settings;
    settings_defaults(&settings);
    if (settings_load(&settings, SETTINGS_FILE) != 0) {
        settings_save(&settings, SETTINGS_FILE);
        logf_("settings.ini criado com os padroes");
    }

    toast("Titans v" APP_VERSION ": iniciou");
    load_system_modules();
    toast("Titans: modulos carregados");

    int video = sceVideoOutOpen(0xFF, 0, 0, NULL);
    if (video < 0) {
        logf_("sceVideoOutOpen failed: %x", (unsigned)video);
        toast("Titans: erro VideoOut %x", (unsigned)video);
        return 1;
    }
    OrbisKernelEqueue flip_queue;
    int32_t rq = sceKernelCreateEqueue(&flip_queue, "titans flip");
    int32_t re = rq == 0 ? sceVideoOutAddFlipEvent(flip_queue, video, NULL) : -1;
    logf_("flip queue: create %x add %x", (unsigned)rq, (unsigned)re);
    off_t phys = 0;
    void* base = NULL;
    int32_t ra = tp_alloc_direct(0, (off_t)tp_direct_size(), 2 * BUFFER_SIZE, 2u << 20, WC_GARLIC, &phys);
    int32_t rm = ra == 0 ? tp_map_direct(&base, 2 * BUFFER_SIZE, PROT_CPU_GPU, 0, phys, 2u << 20) : -1;
    if (ra != 0 || rm != 0) {
        logf_("direct memory failed: alloc %x map %x", (unsigned)ra, (unsigned)rm);
        toast("Titans: erro de memoria %x %x", (unsigned)ra, (unsigned)rm);
        return 1;
    }
    uint8_t* buffers[2] = {(uint8_t*)base, (uint8_t*)base + BUFFER_SIZE};
    void* addresses[2] = {buffers[0], buffers[1]};
    OrbisVideoOutBufferAttribute attribute;
    sceVideoOutSetBufferAttribute(&attribute, ORBIS_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB,
                                  ORBIS_VIDEO_OUT_TILING_MODE_LINEAR, 0, UI_W, UI_H, UI_W);
    int reg = sceVideoOutRegisterBuffers(video, FIRST_BUFFER, addresses, 2, &attribute);
    if (reg < 0) {
        logf_("sceVideoOutRegisterBuffers failed: %x", (unsigned)reg);
        toast("Titans: erro RegisterBuffers %x", (unsigned)reg);
        return 1;
    }
    logf_("video ready");
    toast("Titans: video ok");

    sceUserServiceInitialize(NULL);
    scePadInit();
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0) user = -1;
    int pad = scePadOpen(user, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
    logf_("pad handle %d (user %08x)", pad, (unsigned)user);

    static SysInfo sys;
    sysinfo_init(&sys);
    sysinfo_update(&sys);

    UiState st;
    memset(&st, 0, sizeof st);
    st.settings = &settings;
    st.sys = &sys;
    st.log_text = g_log;
    st.game_ready = exists(GAME_MARK);
    static GameReport game;
    st.game = &game;
    int scan_state = 0; /* 1 = pedido (desenha "analisando"), 2 = roda a analise no proximo quadro */

    uint32_t prev = 0;
    int held[4] = {0, 0, 0, 0}; /* UP DOWN LEFT RIGHT: repeticao ao segurar */
    static const uint32_t nav_btn[4] = {ORBIS_PAD_BUTTON_UP, ORBIS_PAD_BUTTON_DOWN, ORBIS_PAD_BUTTON_LEFT,
                                        ORBIS_PAD_BUTTON_RIGHT};
    int k = 0, quit = 0, frames = 0;
    double t_fps = now_sec(), t_sys = t_fps;
    for (unsigned frame = 0; !quit; frame++) {
        if (pad < 0 && frame % 30 == 0) pad = scePadOpen(user, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
        if (frame % 60 == 0) st.game_ready = exists(GAME_MARK);

        OrbisPadData d;
        memset(&d, 0, sizeof d);
        uint32_t buttons = 0;
        if (pad >= 0 && scePadReadState(pad, &d) == 0 && d.connected) buttons = d.buttons;
        uint32_t pressed = buttons & ~prev;
        prev = buttons;
        int nav[4];
        for (int i = 0; i < 4; i++) {
            held[i] = (buttons & nav_btn[i]) ? held[i] + 1 : 0;
            nav[i] = held[i] == 1 || (held[i] > 16 && (held[i] - 16) % 4 == 0);
        }
        int up = nav[0], down = nav[1], left = nav[2], right = nav[3];
        int cross = (pressed & ORBIS_PAD_BUTTON_CROSS) != 0, circle = (pressed & ORBIS_PAD_BUTTON_CIRCLE) != 0;
        int triangle = (pressed & ORBIS_PAD_BUTTON_TRIANGLE) != 0;

        if (st.screen == SCREEN_MAIN) {
            if (up) st.selected = (st.selected + MENU_COUNT - 1) % MENU_COUNT;
            if (down) st.selected = (st.selected + 1) % MENU_COUNT;
            if (cross) {
                st.message = NULL;
                if (st.selected == MENU_START) {
                    logf_("start pressed: analisando o jogo (runtime fase 1)");
                    st.screen = SCREEN_GAME;
                    st.game_busy = 1;
                    scan_state = 1;
                } else if (st.selected == MENU_SETTINGS) {
                    st.screen = SCREEN_SETTINGS;
                } else if (st.selected == MENU_LOG) {
                    st.log_prev = 0;
                    st.log_scroll = 0;
                    load_log(0);
                    st.screen = SCREEN_LOG;
                } else {
                    quit = 1;
                }
            }
        } else if (st.screen == SCREEN_GAME) {
            if (cross && !st.game_busy) { st.game_busy = 1; scan_state = 1; }
            if (circle) st.screen = SCREEN_MAIN;
        } else if (st.screen == SCREEN_SETTINGS) {
            if (up) st.set_sel = (st.set_sel + S_COUNT) % (S_COUNT + 1);
            if (down) st.set_sel = (st.set_sel + 1) % (S_COUNT + 1);
            int step = right || cross ? 1 : (left ? -1 : 0);
            if (step && st.set_sel == S_COUNT) {
                if (cross) {
                    settings_defaults(&settings);
                    settings_save(&settings, SETTINGS_FILE);
                    logf_("settings: padroes restaurados");
                }
            } else if (step) {
                int n = k_settings[st.set_sel].count;
                settings.v[st.set_sel] = (settings.v[st.set_sel] + step + n) % n;
                settings_save(&settings, SETTINGS_FILE);
            }
            if (circle) st.screen = SCREEN_MAIN;
        } else {
            int total = ui_count_lines(g_log), vis = ui_log_visible_lines();
            int max_scroll = total > vis ? total - vis : 0;
            if (up) st.log_scroll = clampi(st.log_scroll + 1, 0, max_scroll);
            if (down) st.log_scroll = clampi(st.log_scroll - 1, 0, max_scroll);
            if (triangle) {
                st.log_prev = !st.log_prev;
                st.log_scroll = 0;
                load_log(st.log_prev);
            }
            if (circle) st.screen = SCREEN_MAIN;
        }

        if (scan_state == 2) {
            gamescan_run(&game, GAME_DIR, GAME_INI, REPORT_FILE);
            logf_("analise: exe=%s pe_ok=%d dlls=%d funcs=%d arquivos=%d", game.exe[0] ? game.exe : "-", game.pe.ok, game.pe.dlls,
                  game.pe.funcs, game.files);
            st.game_busy = 0;
            scan_state = 0;
        }
        double t = now_sec();
        frames++;
        if (t - t_fps >= 0.5) {
            sys.fps = frames / (t - t_fps);
            frames = 0;
            t_fps = t;
        }
        if (t - t_sys >= 1.0) {
            sysinfo_update(&sys);
            t_sys = t;
        }

        ui_draw(g_frame, &st);
        memcpy(buffers[k], g_frame, sizeof g_frame);
        __atomic_thread_fence(__ATOMIC_SEQ_CST); /* stores write-combined saem antes do flip */
        sceVideoOutSubmitFlip(video, FIRST_BUFFER + k, ORBIS_VIDEO_OUT_FLIP_VSYNC, 0);
        if (frame == 0) {
            sceSystemServiceHideSplashScreen();
            toast("Titans: menu rodando");
        }
        k ^= 1;
        if (scan_state == 1) scan_state = 2;
        sceKernelUsleep(16000); /* o outro buffer ja saiu da tela */
    }
    logf_("exit");
    return 0;
}
