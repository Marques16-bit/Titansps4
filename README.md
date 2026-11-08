# Titans of the Tide — PS4 (projeto de teste)

Tudo roda no GitHub Actions; nada precisa de computador.

## Estado real (leia primeiro)
- **Funciona (a testar no seu PS4):** o app-menu. Gera um `.pkg` com tela própria, controle DualShock 4 e 3 botões:
  **Iniciar** (só habilita se existir `/data/titans-of-the-tide/game/Engine`), **Criar caminho** (cria a pasta do jogo) e **Exit**.
- **Ainda não existe:** o runtime que abre o jogo. Rodar a UE5 do PC no PS4 exige uma camada Windows→PS4 e
  D3D12/Vulkan→GPU do PS4 (veja o SMW como referência de escala). O botão **Iniciar** só avisa isso.
- **Patch gráfico:** o workflow `PS4 Patch` gera os `.ini` com 900p, render scale 75%, 30 FPS e Medium. Eles só têm efeito
  quando o runtime existir para aplicá-los.
- O build nunca foi rodado fora do GitHub (o ambiente onde isto foi montado não tem a toolchain nem rede). Se a primeira
  execução falhar, baixe o artefato `build-log` e mande o `build.log`.

## Passo a passo (celular, pelo navegador)
1. Crie `.github/workflows/extract_zip.yml` (Add file → Create new file; cole o conteúdo de `workflows/extract_zip.yml`).
2. Envie este zip para a raiz do repo com o nome **`repo.zip`** (Add file → Upload files). O workflow *Extrair repo.zip*
   roda sozinho e espalha tudo na raiz.
3. O Actions não pode criar workflows. Mova à mão: abra `workflows/build_pkg.yml` → lápis → no campo do nome troque para
   `.github/workflows/build_pkg.yml` → Commit. Repita com `ps4_patch.yml`.
4. Aba **Actions → Build PKG → Run workflow**. No fim, baixe o artefato **ps4-pkg** (`Titans-PS4-vXX.XX.pkg`).
   O artefato **build-log** traz o `build.log` e um PNG do menu.
5. Instale o `.pkg` com o Package Installer do GoldHEN. Para o jogo: **Criar caminho** no app, depois copie os arquivos por FTP
   para `/data/titans-of-the-tide/game/` (deve existir `game/Engine`). Log do app: `/data/titans-of-the-tide.log`.

## Patch gráfico (opcional)
Coloque os `.ini` originais do jogo em `config/original/` (os que vêm no zip são só amostra) e rode
**Actions → PS4 Patch**. O artefato `ps4-config-patched` traz `DefaultEngine.ini`, `Engine.ini` e `GameUserSettings.ini`.

## Pastas
- `app/` código do app (C) e `app/sce_sys/icon0.png`
- `scripts/` toolchain, empacotador do `.pkg` e `apply_ue_cvars.py`
- `workflows/` os 3 workflows (copiar para `.github/workflows/`)
- `tools/` gerador da fonte e prévia do menu no PC
- `config/original/` .ini originais do jogo
