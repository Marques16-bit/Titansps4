#!/usr/bin/env bash
# Monta o PKG falso (fake PKG) do PS4 a partir de uma pasta com eboot.bin e sce_sys/icon0.png.
#   make-pkg.sh <pasta-stage> <TITLE_ID> <titulo> <versao XX.YY> <rotulo> <pasta-de-saida>
# Imprime o caminho do .pkg na ultima linha; o resto do log vai para o stderr.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OPENORBIS="${OPENORBIS:-/opt/pacbrew/ps4/openorbis}"
TOOLS="$OPENORBIS/bin/linux"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

[ "$#" -eq 6 ] || { echo "uso: make-pkg.sh <stage> <TITLE_ID> <titulo> <versao> <rotulo> <saida>" >&2; exit 1; }
STAGE="$(readlink -f "$1")"; TITLE_ID="$2"; TITLE="$3"; VERSION="$4"; LABEL="$5"
mkdir -p "$6"; OUT="$(readlink -f "$6")"

[[ "$TITLE_ID" =~ ^[A-Z]{4}[0-9]{5}$ ]] || { echo "TITLE_ID deve ser do tipo ABCD12345" >&2; exit 1; }
[[ "$VERSION" =~ ^[0-9]{2}\.[0-9]{2}$ ]] || { echo "versao deve ser do tipo 00.02" >&2; exit 1; }
for f in eboot.bin sce_sys/icon0.png; do
    [ -f "$STAGE/$f" ] || { echo "falta $STAGE/$f" >&2; exit 1; }
done

# Modulos do OpenOrbis em sce_module/ (libc.prx, libSceFios2.prx): copiados da pasta de exemplo da toolchain.
# Sem eles o PS4 recusa o app. (right.sprx nao e usado.)
mkdir -p "$STAGE/sce_module"
SAMPLE_MODULES="$OPENORBIS/samples/piglet/sce_module"
if compgen -G "$SAMPLE_MODULES/*.prx" >/dev/null; then
    cp "$SAMPLE_MODULES"/*.prx "$STAGE/sce_module/"
else
    for prx in libc.prx libSceFios2.prx; do
        src="$(find "$OPENORBIS" -path "*/sce_module/$prx" -print -quit 2>/dev/null || true)"
        [ -n "$src" ] && cp "$src" "$STAGE/sce_module/$prx"
    done
fi
for prx in libc.prx libSceFios2.prx; do
    if [ ! -f "$STAGE/sce_module/$prx" ]; then
        echo "erro: $prx nao esta na toolchain ($OPENORBIS). Modulos .prx encontrados:" >&2
        find "$OPENORBIS" -name '*.prx' >&2 || true
        exit 1
    fi
done
echo "sce_module: $(ls "$STAGE/sce_module" | tr '\n' ' ')" >&2

# CONTENT_ID = IV0000-<TITLE_ID>_00-<rotulo de 16 caracteres, completado com 0>
label="$(printf '%s' "$LABEL" | tr '[:lower:]' '[:upper:]' | tr -cd 'A-Z0-9' | cut -c1-16)"
label="$(printf '%-16s' "$label" | tr ' ' '0')"
CONTENT_ID="IV0000-${TITLE_ID}_00-${label}"

SFO="$STAGE/sce_sys/param.sfo"
rm -f "$SFO"
"$TOOLS/PkgTool.Core" sfo_new "$SFO"
# nome | tipo | tamanho maximo | valor
entries=(
    "APP_TYPE|Integer|4|1"
    "ATTRIBUTE|Integer|4|0x2"
    "CATEGORY|Utf8|4|gd"
    "SYSTEM_VER|Integer|4|0"
    "APP_VER|Utf8|8|$VERSION"
    "CONTENT_ID|Utf8|48|$CONTENT_ID"
    "DOWNLOAD_DATA_SIZE|Integer|4|0"
    "TITLE|Utf8|128|$TITLE"
    "TITLE_ID|Utf8|12|$TITLE_ID"
    "VERSION|Utf8|8|$VERSION"
)
for e in "${entries[@]}"; do
    IFS='|' read -r name type size value <<<"$e"
    "$TOOLS/PkgTool.Core" sfo_setentry "$SFO" "$name" --type "$type" --maxsize "$size" --value "$value" >/dev/null
done
echo "--- param.sfo:" >&2
"$TOOLS/PkgTool.Core" sfo_listentries "$SFO" >&2 || true

# o PkgTool quebra com pastas vazias
find "$STAGE" -type d -empty -delete
rm -f "$STAGE/pkg.gp4"
( cd "$STAGE" && "$TOOLS/create-gp4" -out pkg.gp4 --content-id="$CONTENT_ID" --path . ) >/dev/null
python3 "$HERE/gp4_dirs.py" "$STAGE/pkg.gp4" >&2
echo "--- conteudo do pacote:" >&2
( cd "$STAGE" && find . -type f -exec ls -l {} \; ) >&2
( cd "$STAGE" && "$TOOLS/PkgTool.Core" pkg_build pkg.gp4 "$OUT" ) >/dev/null
rm -f "$STAGE/pkg.gp4"
echo "$OUT/$CONTENT_ID.pkg"
