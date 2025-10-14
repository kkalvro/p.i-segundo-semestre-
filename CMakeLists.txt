cmake_minimum_required(VERSION 3.10)
project(projeto_jogo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Procurar Allegro
find_package(PkgConfig REQUIRED)

pkg_check_modules(ALLEGRO REQUIRED
    allegro-5
    allegro_main-5
    allegro_primitives-5
    allegro_font-5
    allegro_ttf-5
    allegro_image-5
    allegro_dialog-5
)

# Criar executável
add_executable(main "Integrando todos os códigos.c")
target_include_directories(main PRIVATE ${ALLEGRO_INCLUDE_DIRS})
target_link_directories(main PRIVATE ${ALLEGRO_LIBRARY_DIRS})
target_link_libraries(main ${ALLEGRO_LIBRARIES})

# Copiar assets para build
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()