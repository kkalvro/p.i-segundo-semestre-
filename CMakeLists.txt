cmake_minimum_required(VERSION 3.10)

project(p.i-segundo-semestre- C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO REQUIRED
    allegro-5
    allegro_main-5
    allegro_primitives-5
    allegro_font-5
    allegro_ttf-5
    allegro_image-5
    allegro_audio-5
    allegro_acodec-5
)

add_executable(al_demo main.c
)

target_include_directories(al_demo PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(al_demo PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(al_demo ${ALLEGRO_LIBRARIES})