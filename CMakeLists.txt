cmake_minimum_required(VERSION 3.10)
project(projeto_jogo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -----------------------------
# Configuração para Apple Silicon
# -----------------------------
if(APPLE)
    set(HOMEBREW_PREFIX "/opt/homebrew")
    set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/lib/pkgconfig")
    set(CMAKE_PREFIX_PATH ${HOMEBREW_PREFIX})
    include_directories(${HOMEBREW_PREFIX}/include)
    link_directories(${HOMEBREW_PREFIX}/lib)
endif()

# -----------------------------
# Procurar Allegro
# -----------------------------
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
    allegro_dialog-5
)

message(STATUS "Allegro found: ${ALLEGRO_FOUND}")

# -----------------------------
# Criar executáveis separados
# -----------------------------

# Adivinha (fase 2.c)
add_executable(main "Integrando todos os códigos.c")
target_include_directories(main PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(main PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(main ${ALLEGRO_LIBRARIES})


# -----------------------------
# Copiar assets para build
# -----------------------------

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()