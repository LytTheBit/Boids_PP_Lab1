cmake_minimum_required(VERSION 3.20)
project(Boids_PP_Lab1 LANGUAGES CXX)

# Uso C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Percorso della configurazione SFML
set(SFML_DIR "C:/Users/Utente/SFML-2.6.2/lib/cmake/SFML")

# Cerca SFML (graphics + window + system)
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# --- OpenMP ---
find_package(OpenMP REQUIRED)

# Indica l'eseguibile
add_executable(Boids_PP_Lab1
        main.cpp
        boid.hpp
        config.hpp
        boid.cpp
)

# Collega le librerie SFML + OpenMP
target_link_libraries(Boids_PP_Lab1 PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        OpenMP::OpenMP_CXX
)


