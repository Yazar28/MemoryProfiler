cmake_minimum_required(VERSION 3.21)
project(MemoryProfiler LANGUAGES CXX)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opciones específicas de MSVC
if(MSVC)
    add_compile_options(/W4 /EHsc /MP /Zc:__cplusplus)  # Warnings altos, excepciones, multi-core, __cplusplus
endif()

# Ruta de Qt6
set(Qt6_DIR "C:/Qt/6.9.2/msvc2022_64/lib/cmake/Qt6")  # Donde CMake encuentra los módulos de Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Directorios de salida
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # Ejecutables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Bibliotecas dinámicas
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Bibliotecas estáticas

# Subdirectorios del proyecto
add_subdirectory(MemoryProfiler)
add_subdirectory(gui)
add_subdirectory(tests)
add_subdirectory(DataStructure)

# Mensajes informativos
message(STATUS "Qt6 encontrado en: ${Qt6_DIR}")
message(STATUS "Compilador: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Versión C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Archivos de build generados en: ${CMAKE_BINARY_DIR}")
