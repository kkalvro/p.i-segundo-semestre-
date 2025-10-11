cmake_minimum_required(VERSION 3.10)
project(projeto_jogo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configurar caminhos para Homebrew (macOS)
set(CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

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

add_executable(projeto_jogo main.c)

# Configurações de include, link e flags
target_include_directories(projeto_jogo PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(projeto_jogo PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(projeto_jogo ${ALLEGRO_LIBRARIES})
target_compile_options(projeto_jogo PRIVATE ${ALLEGRO_CFLAGS_OTHER})

# IMPORTANTE: Copiar pasta assets para o diretório de build
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})