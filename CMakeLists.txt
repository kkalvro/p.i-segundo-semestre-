cmake_minimum_required(VERSION 3.10)
project(untitled1 C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO REQUIRED allegro-5 allegro_color-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5)

add_executable(untitled1 main.c)

target_include_directories(untitled1 PRIVATE ${ALLEGRO_INCLUDE_DIRS})

target_link_libraries(untitled1 ${ALLEGRO_LIBRARIES})
