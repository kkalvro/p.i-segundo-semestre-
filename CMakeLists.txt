cmake_minimum_required(VERSION 3.10)
project(game C)

set(CMAKE_C_STANDARD 11)

# Cria o executável
add_executable(game all.c)

# Busca Allegro
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO REQUIRED
    allegro-5 allegro_main-5 allegro_image-5
    allegro_primitives-5 allegro_font-5 allegro_ttf-5
    allegro_audio-5 allegro_acodec-5 allegro_dialog-5
)

# Configura includes e bibliotecas
target_include_directories(game PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(game PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(game PRIVATE ${ALLEGRO_LIBRARIES}
    "-framework Cocoa" "-framework IOKit" "-framework CoreVideo"
    "-framework CoreAudio" "-framework AudioToolbox"
)