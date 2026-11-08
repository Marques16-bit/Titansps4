# Uso: source scripts/ps4-env.sh
# Carrega a toolchain OpenOrbis (PacBrew) e define cmake_ps4, o cmake ja apontado para a toolchain do PS4.
OPENORBIS="${OPENORBIS:-/opt/pacbrew/ps4/openorbis}"
export OPENORBIS
if [ ! -f "$OPENORBIS/ps4vars.sh" ]; then
    echo "toolchain nao encontrada em $OPENORBIS (rode: sudo bash scripts/fetch-pacbrew.sh /)" >&2
    return 1
fi
# shellcheck disable=SC1091
source "$OPENORBIS/ps4vars.sh"

# O ld.lld do PacBrew pede libxml2.so.2; distribuicoes novas trazem a mesma biblioteca com outro numero.
if ldd "$OPENORBIS/bin/ld.lld" 2>/dev/null | grep -q 'libxml2.so.2 => not found'; then
    for lib in /usr/lib/x86_64-linux-gnu/libxml2.so.*; do
        case "$lib" in
            *.so.[0-9] | *.so.[0-9][0-9])
                mkdir -p "$HOME/ps4-host-compat"
                ln -sf "$lib" "$HOME/ps4-host-compat/libxml2.so.2"
                export LD_LIBRARY_PATH="$HOME/ps4-host-compat${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
                break
                ;;
        esac
    done
fi

# O ps4vars.sh exporta flags para makefiles; o cmake os somaria aos da toolchain, entao saem do ambiente dele.
cmake_ps4() {
    env -u CFLAGS -u CXXFLAGS -u CPPFLAGS -u LDFLAGS -u LIBS \
        cmake -DCMAKE_TOOLCHAIN_FILE="$OPENORBIS/cmake/ps4.cmake" \
              -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 "$@"
}
