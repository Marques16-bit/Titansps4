# ps4_add_eboot(<alvo>): converte o ELF do alvo em <build>/<alvo>_eboot/eboot.bin com o create-fself do OpenOrbis e
# grava o caminho do ELF em <build>/<alvo>.path (o build-pkg.sh usa para gerar a segunda variante de identidade).
if(NOT DEFINED OPENORBIS)
    if(DEFINED ENV{OPENORBIS})
        set(OPENORBIS $ENV{OPENORBIS})
    else()
        set(OPENORBIS /opt/pacbrew/ps4/openorbis)
    endif()
endif()

# Identidade do eboot (variante A): PAID e authinfo do RetroArch for PS4, valores de apps homebrew que abrem video e
# controle. Sem uma identidade que o sistema aceite, o carregamento dos modulos de video/controle e negado e o app
# fecha sem mensagem.
set(PS4_PAID "0x3100000000000002" CACHE STRING "Program authority ID do eboot.bin")
set(PS4_AUTH_INFO "000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" CACHE STRING "Authinfo do eboot.bin (hex)")

function(ps4_add_eboot target)
    set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}_eboot)
    add_custom_command(
        OUTPUT ${out_dir}/eboot.bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
        COMMAND ${CMAKE_COMMAND} -E env OO_PS4_TOOLCHAIN=${OPENORBIS}
            ${OPENORBIS}/bin/create-fself -in=$<TARGET_FILE:${target}>
            -out=${out_dir}/${target}.oelf --eboot ${out_dir}/eboot.bin
            --paid ${PS4_PAID} --authinfo ${PS4_AUTH_INFO}
        DEPENDS ${target}
        VERBATIM)
    add_custom_target(${target}_eboot ALL DEPENDS ${out_dir}/eboot.bin)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}.path CONTENT "$<TARGET_FILE:${target}>")
endfunction()
