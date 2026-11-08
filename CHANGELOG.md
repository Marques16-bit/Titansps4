# Changelog

## v00.06
- Causa do app fechar sozinho: o `malloc` padrão do OpenOrbis reserva um bloco enorme de memória na primeira alocação e
  derruba o app quando isso falha (acontece em alguns firmwares). O `fopen` aloca memória, então o app caía já no primeiro
  log: os `app.log` ficavam com 0 bytes.
- O app não usa mais `malloc` nem stdio de arquivos: log, `settings.ini` e leitura do log usam `open/read/write` direto.

## v00.05
- Volta a gerar um único `.pkg`.
- Sequência de inicialização do vídeo igual à do app de referência: módulos → VideoOut → fila de eventos de flip →
  buffers a partir do índice 12 → só depois UserService/Pad.
- Módulos do sistema copiados da pasta de exemplo da toolchain; `right.sprx` removido do pacote.
- Avisos na tela (toasts) a cada etapa da inicialização e no crash, quando a toolchain permite (`build.log`: `probe`).

## v00.04
- O app ainda fechava sozinho: o `eboot.bin` agora é assinado com PAID e authinfo explícitos (o padrão da ferramenta não
  garante acesso aos módulos de vídeo e controle). O artefato traz duas variantes: A (PAID 0x31...02 + authinfo) e
  B (PAID 0x38...11, o dos exemplos OpenOrbis). O título do app no PS4 mostra (A) ou (B).

## v00.03
- Corrige o app que abria e fechava sozinho: agora carrega os módulos do sistema (SystemService, UserService, VideoOut, Pad)
  antes de usá-los. Chamar uma biblioteca sem o módulo carregado fecha o app sem mensagem.
- Menu vertical Start / Settings / Log / Exit, com capa nova (`icon0.png`, `pic1.png`) e emblema no app.
- Settings com 23 opções (vídeo, overlay, áudio, controles, sistema) salvas em `/data/Titans/config/settings.ini`.
- Overlay opcional: FPS, RAM, temperatura e armazenamento. Tela Log com a execução atual e a anterior.
- Estrutura de pastas `/data/Titans/{game,config,logs,saves,cache/shaders}`, criada ao abrir.
- Log de falhas: registra cada etapa da inicialização e o sinal de crash (SIGSEGV etc.), via `signal()` padrão.
- O pacote leva também `sce_sys/about/right.sprx` quando a toolchain traz.
- Removido o patch de `.ini` (workflow, script e `config/`).

## v00.02
- Pacote com `sce_module/libc.prx` e `sce_module/libSceFios2.prx`; versão no app; `build.log` com conteúdo do pacote.

## v00.01
- Primeiro app-menu e workflows.
