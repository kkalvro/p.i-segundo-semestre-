cmake_minimum_required(VERSION 3.10)
project(p.i-segundo-semestre- C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO REQUIRED allegro-5 allegro_color-5 allegro_image-5 allegro_dialog-5)

# Cria o executável com o mesmo nome do projeto
add_executable(p.i-segundo-semestre- main.c
        main.c)

# Inclui os headers do Allegro
target_include_directories(p.i-segundo-semestre- PRIVATE ${ALLEGRO_INCLUDE_DIRS})

# Linka as libs do Allegro
target_link_libraries(p.i-segundo-semestre- PRIVATE ${ALLEGRO_LIBRARIES})
