# ELF -> <build>/<alvo>_eboot/eboot.bin (fake SELF) e confere o resultado.
# Adaptado de toolchain/ps4-love-style.cmake do SMW (iHaiDeeZ/SMW, MIT): mesma chamada do create-fself,
# sem PAID/authinfo (identidade homebrew padrao do OpenOrbis, que o SMW confirma que abre).
if(NOT DEFINED OPENORBIS)
    if(DEFINED ENV{OPENORBIS})
        set(OPENORBIS $ENV{OPENORBIS})
    else()
        set(OPENORBIS /opt/pacbrew/ps4/openorbis)
    endif()
endif()
set(TITANS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

function(ps4_add_eboot target)
    set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}_eboot)
    add_custom_command(
        OUTPUT ${out_dir}/eboot.bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
        COMMAND ${CMAKE_COMMAND} -E env OO_PS4_TOOLCHAIN=${OPENORBIS}
            ${OPENORBIS}/bin/create-fself -in=$<TARGET_FILE:${target}>
            -out=${out_dir}/${target}.oelf --eboot ${out_dir}/eboot.bin
        COMMAND ${CMAKE_COMMAND} -DOELF=${out_dir}/${target}.oelf -P ${TITANS_CMAKE_DIR}/check-oelf.cmake
        DEPENDS ${target}
        VERBATIM)
    add_custom_target(${target}_eboot ALL DEPENDS ${out_dir}/eboot.bin)
endfunction()
