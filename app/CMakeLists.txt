cmake_minimum_required(VERSION 3.13)
project(titans_ps4 C)

# CPU do PS4 (AMD Jaguar)
add_compile_options(-march=btver2 -O2 -Wall)

set(APP_VERSION "dev" CACHE STRING "Versao mostrada no app")
add_executable(titans src/main.c src/ui.c src/settings.c src/sysinfo.c)
target_compile_definitions(titans PRIVATE APP_VERSION="${APP_VERSION}")
target_link_libraries(titans PRIVATE SceVideoOut ScePad SceUserService SceSystemService SceSysmodule)

# APIs opcionais do kernel: build-pkg.sh passa -DHAVE_...=1 so quando a toolchain exporta o simbolo
foreach(feat HAVE_KERNEL_CPU_TEMP HAVE_KERNEL_SOC_TEMP HAVE_KERNEL_STATFS HAVE_KERNEL_NOTIFY)
    if(${feat})
        target_compile_definitions(titans PRIVATE ${feat}=1)
    endif()
endforeach()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/eboot.cmake)
ps4_add_eboot(titans)
