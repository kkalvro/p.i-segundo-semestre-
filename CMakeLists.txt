cmake_minimum_required(VERSION 3.16)   # Diz ao CMake: "não rode com versão menor que 3.16".
                                        # Isso garante que recursos modernos (ex.: target_link_directories)
                                        # existam. Se a IDE tiver CMake antigo, ele aborta aqui.

project(allegro_hello C)                # Cria um "projeto" chamado allegro_hello e informa a linguagem: C.
                                        # Internamente, o CMake define variáveis como PROJECT_NAME, etc.,
                                        # e ativa a toolchain C (clang) detectada pelo CLion.

set(CMAKE_C_STANDARD 11)                # Pede ao CMake para compilar os binários deste projeto usando C11.
set(CMAKE_C_STANDARD_REQUIRED ON)       # Exige que seja C11 (não aceite “algo parecido”).
# set(CMAKE_C_EXTENSIONS OFF)           # (Opcional) Desliga extensões GNU (ficaria estritamente ISO C11).

find_package(PkgConfig REQUIRED)        # Carrega o módulo "FindPkgConfig" e localiza o executável pkg-config.
                                        # Se não achar, para a configuração com erro (REQUIRED).

# Este comando roda o executável "pkg-config" por baixo dos panos perguntando:
# "Quais CFLAGS, LDFLAGS, include dirs e libs eu preciso para usar Allegro?"
# O resultado preenche variáveis: ALLEGRO_INCLUDE_DIRS, ALLEGRO_LIBRARY_DIRS,
# ALLEGRO_LIBRARIES, ALLEGRO_CFLAGS, ALLEGRO_LDFLAGS, ALLEGRO_VERSION, etc.
pkg_check_modules(ALLEGRO REQUIRED
  allegro-5                             # Núcleo do Allegro (headers + lib principal)
  allegro_main-5                        # No macOS, fornece o "entrypoint" adequado da app (wrapper do main).
)

# (Opcional) Debug: imprime na aba de "CMake" do CLion os caminhos descobertos pelo pkg-config.
# message(STATUS "ALLEGRO_INCLUDE_DIRS=${ALLEGRO_INCLUDE_DIRS}")
# message(STATUS "ALLEGRO_LIBRARY_DIRS=${ALLEGRO_LIBRARY_DIRS}")
# message(STATUS "ALLEGRO_LIBRARIES=${ALLEGRO_LIBRARIES}")

add_executable(al_demo src/main.c)      # Registra o alvo (target) executável "al_demo" e seus fontes.
                                        # Nada é compilado aqui ainda; o CMake só monta o "plano de build".
                                        # A compilação acontece quando você aperta Build (Ninja/Make executam).

# Diz ao compilador (clang) onde procurar os headers do Allegro quando vir "#include <allegro5/...>"
# Isso vira, na prática, flags -I... durante a compilação só do target al_demo.
target_include_directories(al_demo PRIVATE ${ALLEGRO_INCLUDE_DIRS})

# Diz ao *linker* onde estão as bibliotecas (.dylib) do Allegro. Vira -L... na etapa de link.
# Usamos a variante por-target (target_link_directories) para não "poluir" o projeto inteiro.
target_link_directories(al_demo PRIVATE ${ALLEGRO_LIBRARY_DIRS})

# Manda efetivamente linkar contra as libs que o pkg-config reportou (nomes corretos e na ordem certa),
# por exemplo: -lallegro_main-5.2 -lallegro-5.2 ... (podem vir extras, tipo -lpng, -lpthread, etc.)
target_link_libraries(al_demo ${ALLEGRO_LIBRARIES})

# (Opcional, mas útil no macOS) Embute no executável a "rpath" de onde achar as .dylib em runtime.
# Sem isso, às vezes o dyld reclama "Library not loaded". Você pode usar CMAKE_BUILD_RPATH global
# ou, mais fino, por-target como abaixo.
# set_target_properties(al_demo PROPERTIES
#   BUILD_RPATH "${ALLEGRO_LIBRARY_DIRS}"
# )