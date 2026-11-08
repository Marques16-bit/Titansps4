cmake_minimum_required(VERSION 3.13)
project(titans_ps4 C)

# CPU do PS4 (AMD Jaguar): o mesmo -march que o SMW usa
add_compile_options(-march=btver2 -O2 -Wall)

add_executable(titans src/main.c src/ui.c)
target_link_libraries(titans PRIVATE SceVideoOut ScePad SceUserService SceSystemService)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/eboot.cmake)
ps4_add_eboot(titans)
