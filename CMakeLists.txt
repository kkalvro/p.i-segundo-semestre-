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

# Fase Final (main.c)
add_executable(final main.c)
target_include_directories(final PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(final PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(final ${ALLEGRO_LIBRARIES})

# Lobby (lobby.c)
add_executable(lobby lobby.c)
target_include_directories(lobby PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(lobby PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(lobby ${ALLEGRO_LIBRARIES})

# Adivinha (fase 2.c)
add_executable(adivinha "fase 2.c"
        bobbie_goods.c)
target_include_directories(adivinha PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(adivinha PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(adivinha ${ALLEGRO_LIBRARIES})

# Quebra Cabeça (quebra_cabeca)
add_executable(quebra_cabeca quebra_cabeca.c)
target_include_directories(quebra_cabeca PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(quebra_cabeca PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(quebra_cabeca ${ALLEGRO_LIBRARIES})

# Menu (menu_do_jogo.c)
add_executable(menu menu_do_jogo.c)
target_include_directories(menu PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(menu PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(menu ${ALLEGRO_LIBRARIES})

# Desenho (bobbie_goods.c)
add_executable(desenho bobbie_goods.c)
target_include_directories(desenho PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(desenho PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(desenho ${ALLEGRO_LIBRARIES})


# -----------------------------
# Copiar assets para build
# -----------------------------

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()