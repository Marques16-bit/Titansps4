#!/usr/bin/env bash
# Compila o app e gera o .pkg:  PKG_VERSION=01.00 bash scripts/build-pkg.sh
# Pre-requisito: toolchain em /opt/pacbrew (bash scripts/fetch-pacbrew.sh /, como root).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${PKG_VERSION:-01.00}"
TITLE_ID="${PKG_TITLE_ID:-TITA00001}"
source "$ROOT/scripts/ps4-env.sh"

BUILD="$ROOT/build"
rm -rf "$BUILD"
mkdir -p "$BUILD"
# APIs opcionais do kernel (temperatura, espaco em disco): so entram se os stubs da toolchain exportam o simbolo.
probe_symbol() {
    local sym="$1" lib out
    while IFS= read -r lib; do
        out="$( { nm -D "$lib"; nm "$lib"; } 2>/dev/null || true )"
        if grep -qw -- "$sym" <<<"$out"; then return 0; fi
    done < <(find "$OPENORBIS" \( -name 'libkernel*.so' -o -name 'libkernel*.a' \) 2>/dev/null)
    return 1
}
FEATURES=()
for pair in HAVE_KERNEL_CPU_TEMP:sceKernelGetCpuTemperature HAVE_KERNEL_SOC_TEMP:sceKernelGetSocSensorTemperature HAVE_KERNEL_STATFS:statfs; do
    flag="${pair%%:*}"; sym="${pair##*:}"
    if probe_symbol "$sym"; then FEATURES+=("-D$flag=1"); echo "probe $sym: sim"; else echo "probe $sym: nao (o app mostra n/a)"; fi
done
cmake_ps4 -S "$ROOT/app" -B "$BUILD" -DAPP_VERSION="$VERSION" "${FEATURES[@]}"
cmake --build "$BUILD" --parallel
command -v readelf >/dev/null && readelf -d "$BUILD/titans_eboot/titans.oelf" | grep -E 'NEEDED|SONAME' || true
python3 "$ROOT/scripts/check_oelf.py" "$BUILD/titans_eboot/titans.oelf"
EBOOT="$BUILD/titans_eboot/eboot.bin"
[ -f "$EBOOT" ] || { echo "eboot.bin nao foi gerado" >&2; exit 1; }

STAGE="$BUILD/stage"
mkdir -p "$STAGE/sce_sys"
cp "$EBOOT" "$STAGE/eboot.bin"
cp "$ROOT"/app/sce_sys/*.png "$STAGE/sce_sys/"

PKG="$(bash "$ROOT/scripts/make-pkg.sh" "$STAGE" "$TITLE_ID" "Titans of the Tide" "$VERSION" TITANSOFTHETIDE "$BUILD/pkg-out" | tail -1)"
mkdir -p "$ROOT/out"
cp "$PKG" "$ROOT/out/Titans-PS4-v$VERSION.pkg"
ls -la "$ROOT/out"
