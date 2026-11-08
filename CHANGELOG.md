# Changelog

## v00.02
- Limpeza: scripts de empacotamento reescritos (`fetch-pacbrew.sh`, `make-pkg.sh`, `ps4-env.sh`, `gp4_dirs.py`, `check_oelf.py`).
- Corrige o erro de "dados corrompidos" ao abrir: o pacote agora leva `sce_module/libc.prx` e `sce_module/libSceFios2.prx`
  (exigidos pelo OpenOrbis).
- A versão aparece no subtítulo do app e no log `/data/titans-of-the-tide.log`.
- O `build.log` lista o conteúdo do pacote e o `param.sfo`.

## v00.01
- Primeiro app-menu (tela, controle, Criar caminho) e workflows.
