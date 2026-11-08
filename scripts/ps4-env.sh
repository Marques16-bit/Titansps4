# Usar com: source scripts/ps4-env.sh
# Adaptado de scripts/ps4-env.sh do SMW (iHaiDeeZ/SMW, MIT): toolchain OpenOrbis do PacBrew (clang 12, ld.lld, create-fself).
OPENORBIS="${OPENORBIS:-/opt/pacbrew/ps4/openorbis}"
export OPENORBIS
# shellcheck disable=SC1091
source "$OPENORBIS/ps4vars.sh"
# O ld.lld do PacBrew liga com libxml2.so.2; distros novas so tem libxml2.so.16 (mesma API): link de compatibilidade.
if ldd "$OPENORBIS/bin/ld.lld" | grep -q "libxml2.so.2 => not found"; then
    compat=$(ls /usr/lib/x86_64-linux-gnu/libxml2.so.* 2>/dev/null | grep -E 'libxml2\.so\.[0-9]+$' | head -1)
    if [ -n "$compat" ]; then
        mkdir -p "$HOME/ps4-host-compat"
        ln -sf "$compat" "$HOME/ps4-host-compat/libxml2.so.2"
        export LD_LIBRARY_PATH="$HOME/ps4-host-compat${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
fi
# ps4vars.sh exporta CFLAGS/LDFLAGS para makefiles; o CMake os somaria aos flags da toolchain: limpa para o CMake.
ps4_cmake() {
    env -u CFLAGS -u CXXFLAGS -u CPPFLAGS -u LDFLAGS -u LIBS \
        cmake -DCMAKE_TOOLCHAIN_FILE="$OPENORBIS/cmake/ps4.cmake" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 "$@"
}
