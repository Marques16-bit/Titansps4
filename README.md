# Titans of the Tide — PS4 (projeto de teste)

Tudo roda no GitHub Actions; nada precisa de computador.

## Estado real (v00.05, veja CHANGELOG.md)
- **O app:** abre com capa e menu vertical **Start / Settings / Log / Exit** (controle DualShock 4).
- **Settings:** opções de vídeo (resolução, render scale, limite de FPS, VSync, texturas, sombras, bloom, SSR, anti-aliasing),
  overlay (FPS, RAM, temperatura, armazenamento), áudio, controles e sistema. Ficam em `/data/Titans/config/settings.ini`.
  Não existe mais patch de `.ini`: o runtime do port vai ler estas opções.
- **Log:** mostra `/data/Titans/logs/app.log` (execução atual) e `app.prev.log` (anterior, com Triângulo).
- **Ainda não existe:** o runtime que abre o jogo. **Start** só avisa isso.
- Temperatura e armazenamento aparecem como `n/a` se o sistema não deixar ler (o `build.log` diz o que foi habilitado).

## Pastas no PS4 (criadas pelo app ao abrir)
```
/data/Titans/
  game/             arquivos do jogo da versão de PC (precisa existir game/Engine)
  config/           settings.ini
  logs/             app.log, app.prev.log
  saves/            saves do jogo
  cache/shaders/    cache de shaders
```

## Passo a passo (celular, pelo navegador)
1. Crie `.github/workflows/extract_zip.yml` (Add file → Create new file; cole `workflows/extract_zip.yml`).
2. Envie este zip para a raiz do repo com o nome **`repo.zip`**. O workflow *Extrair repo.zip* espalha tudo na raiz.
3. O Actions não cria workflows. Mova à mão `workflows/build_pkg.yml` para `.github/workflows/build_pkg.yml`
   (abra o arquivo → lápis → troque o caminho no campo do nome → Commit).
4. **Actions → Build PKG → Run workflow**, versão `00.05`. Baixe o artefato **ps4-pkg** (um `.pkg`). O **build-log** traz o `build.log`
   e PNGs do menu.
5. Instale o `.pkg` com o Package Installer do GoldHEN. Ao abrir, o PS4 mostra avisos (toasts) "Titans: ..." a cada etapa: se o app fechar, o último aviso diz até onde chegou. Se o PS4 recusar por já ter versão igual ou maior, desinstale o antigo.
6. Se algo falhar ao abrir, pegue por FTP `/data/Titans/logs/app.log` e mande junto com o `build.log`.

Arquivos de versões antigas (`workflows/ps4_patch.yml`, `scripts/apply_ue_cvars.py`, `config/`) continuam no seu repo, pois o
extrator só adiciona e sobrescreve. Pode apagar.

## Pastas do projeto
- `app/` código do app (C) e capa em `app/sce_sys/` (`icon0.png`, `pic1.png`)
- `scripts/` toolchain e empacotador do `.pkg`
- `workflows/` workflows (copiar para `.github/workflows/`)
- `tools/` gerador da fonte, gerador da capa e prévia das telas no PC
