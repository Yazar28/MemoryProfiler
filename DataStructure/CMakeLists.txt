# Configuración mínima de CMake
cmake_minimum_required(VERSION 3.10)
project(DataStructure)

# Configuración de compilación (opcional pero recomendado)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Creamos la librería como header-only
add_library(DataStructure INTERFACE)

# Indicamos dónde están los headers
target_include_directories(DataStructure INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Includes
)

# Ejecutable para pruebas unitarias
add_executable(TestDataStructure TestDataStructure.cpp)

# Enlazamos el ejecutable con la librería
target_link_libraries(TestDataStructure PRIVATE DataStructure)
