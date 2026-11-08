#!/usr/bin/env bash
# Compila o app e gera dois .pkg com identidades de assinatura diferentes (o titulo no menu do PS4 mostra A ou B):
#   A = PAID 0x3100000000000002 + authinfo (app/cmake/eboot.cmake)   B = PAID 0x3800000000000011 (padrao dos exemplos OpenOrbis)
#   PKG_VERSION=00.04 bash scripts/build-pkg.sh
# Pre-requisito: toolchain em /opt/pacbrew (sudo bash scripts/fetch-pacbrew.sh /).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${PKG_VERSION:-00.04}"
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

ELF="$(cat "$BUILD/titans.path")"
[ -f "$ELF" ] || { echo "ELF nao encontrado: $ELF" >&2; exit 1; }
command -v readelf >/dev/null && readelf -d "$ELF" | grep -E 'NEEDED|SONAME' || true

# variante A: eboot do CMake. variante B: mesmo ELF, outra identidade.
python3 "$ROOT/scripts/check_oelf.py" "$BUILD/titans_eboot/titans.oelf"
mkdir -p "$BUILD/variant-b"
env OO_PS4_TOOLCHAIN="$OPENORBIS" "$OPENORBIS/bin/create-fself" -in="$ELF" -out="$BUILD/variant-b/titans.oelf" \
    --eboot "$BUILD/variant-b/eboot.bin" --paid 0x3800000000000011
python3 "$ROOT/scripts/check_oelf.py" "$BUILD/variant-b/titans.oelf"

package() { # <letra> <eboot.bin>
    local tag="$1" eboot="$2" stage="$BUILD/stage-$1" pkg
    mkdir -p "$stage/sce_sys"
    cp "$eboot" "$stage/eboot.bin"
    cp "$ROOT"/app/sce_sys/*.png "$stage/sce_sys/"
    pkg="$(bash "$ROOT/scripts/make-pkg.sh" "$stage" "$TITLE_ID" "Titans of the Tide ($tag)" "$VERSION" TITANSOFTHETIDE "$BUILD/pkg-out-$tag" | tail -1)"
    mkdir -p "$ROOT/out"
    cp "$pkg" "$ROOT/out/Titans-PS4-v$VERSION-$tag.pkg"
}
package A "$BUILD/titans_eboot/eboot.bin"
package B "$BUILD/variant-b/eboot.bin"
ls -la "$ROOT/out"
