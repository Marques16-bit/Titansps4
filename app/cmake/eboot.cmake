# ps4_add_eboot(<alvo>): converte o ELF do alvo em <build>/<alvo>_eboot/eboot.bin com o create-fself do OpenOrbis
# (identidade homebrew padrao, sem PAID/authinfo customizados).
if(NOT DEFINED OPENORBIS)
    if(DEFINED ENV{OPENORBIS})
        set(OPENORBIS $ENV{OPENORBIS})
    else()
        set(OPENORBIS /opt/pacbrew/ps4/openorbis)
    endif()
endif()

function(ps4_add_eboot target)
    set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}_eboot)
    add_custom_command(
        OUTPUT ${out_dir}/eboot.bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
        COMMAND ${CMAKE_COMMAND} -E env OO_PS4_TOOLCHAIN=${OPENORBIS}
            ${OPENORBIS}/bin/create-fself -in=$<TARGET_FILE:${target}>
            -out=${out_dir}/${target}.oelf --eboot ${out_dir}/eboot.bin
        DEPENDS ${target}
        VERBATIM)
    add_custom_target(${target}_eboot ALL DEPENDS ${out_dir}/eboot.bin)
endfunction()
