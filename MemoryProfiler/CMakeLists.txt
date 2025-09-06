# CMakeLists.txt de la librería MemoryProfiler
cmake_minimum_required(VERSION 3.21)

# Crear librería estática
add_library(MemoryProfiler STATIC
    src/MemoryProfiler.cpp
    src/SocketClient.cpp
    src/Traker.cpp
)

# Incluir los headers de la librería
target_include_directories(MemoryProfiler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Estándar de C++
set_target_properties(MemoryProfiler PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Opciones específicas para MSVC
if(MSVC)
    target_compile_options(MemoryProfiler PRIVATE /W4 /EHsc)
endif()
