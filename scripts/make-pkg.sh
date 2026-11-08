#!/usr/bin/env bash
# Empacota uma pasta (eboot.bin + sce_sys/icon0.png) num PKG falso (fake PKG) do PS4.
#   make-pkg.sh <stage-dir> <TITLE_ID> <titulo> <versao XX.YY> <rotulo> <pasta-de-saida>
# Adaptado de scripts/make-pkg.sh do SMW (iHaiDeeZ/SMW, MIT): so o caminho "plain" (identidade homebrew padrao),
# sem os modulos Piglet/Dolphin e sem o PkgTool alterado.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OPENORBIS="${OPENORBIS:-/opt/pacbrew/ps4/openorbis}"

STAGE="$(readlink -f "$1")"; TITLE_ID="$2"; TITLE="$3"; VERSION="$4"; LABEL="$5"; mkdir -p "$6"; OUT="$(readlink -f "$6")"
TOOLS="$OPENORBIS/bin/linux"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

[[ "$TITLE_ID" =~ ^[A-Z]{4}[0-9]{5}$ ]] || { echo "TITLE_ID deve ser tipo ABCD12345" >&2; exit 1; }
[[ "$VERSION" =~ ^[0-9]{2}\.[0-9]{2}$ ]] || { echo "versao deve ser tipo 01.00" >&2; exit 1; }
[ -f "$STAGE/eboot.bin" ] || { echo "falta $STAGE/eboot.bin" >&2; exit 1; }
[ -f "$STAGE/sce_sys/icon0.png" ] || { echo "falta $STAGE/sce_sys/icon0.png" >&2; exit 1; }

LABEL="$(printf '%s' "$LABEL" | tr '[:lower:]' '[:upper:]' | tr -cd 'A-Z0-9')"
LABEL="$(printf '%-16s' "${LABEL:0:16}" | tr ' ' '0')"
CONTENT_ID="IV0000-${TITLE_ID}_00-${LABEL}"

SFO="$STAGE/sce_sys/param.sfo"
rm -f "$SFO"
"$TOOLS/PkgTool.Core" sfo_new "$SFO"
set_entry() { "$TOOLS/PkgTool.Core" sfo_setentry "$SFO" "$1" --type "$2" --maxsize "$3" --value "$4" >/dev/null; }
# Valores do OpenOrbis/PSChrome (abrem com o PAID padrao). 0x2: dialogos do sistema confirmam com Cross.
set_entry APP_TYPE Integer 4 1
set_entry ATTRIBUTE Integer 4 0x2
set_entry CATEGORY Utf8 4 gd
set_entry SYSTEM_VER Integer 4 0
set_entry APP_VER Utf8 8 "$VERSION"
set_entry CONTENT_ID Utf8 48 "$CONTENT_ID"
set_entry DOWNLOAD_DATA_SIZE Integer 4 0
set_entry TITLE Utf8 128 "$TITLE"
set_entry TITLE_ID Utf8 12 "$TITLE_ID"
set_entry VERSION Utf8 8 "$VERSION"

# o PkgTool quebra com pastas vazias
find "$STAGE" -type d -empty -delete
rm -f "$STAGE/pkg.gp4"
( cd "$STAGE" && "$TOOLS/create-gp4" -out pkg.gp4 --content-id="$CONTENT_ID" --path . ) >/dev/null
python3 "$HERE/gp4-fix-dirs.py" "$STAGE/pkg.gp4" >&2
( cd "$STAGE" && "$TOOLS/PkgTool.Core" pkg_build pkg.gp4 "$OUT" ) >/dev/null
rm -f "$STAGE/pkg.gp4"
echo "$OUT/$CONTENT_ID.pkg"
