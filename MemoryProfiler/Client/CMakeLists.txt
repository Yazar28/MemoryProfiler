cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Configurar directorios de salida
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Biblioteca del cliente (ServerClient)
add_library(ServerClient STATIC
    ServerClient.cpp
    ServerClient.h
    profiler_structures.h
)

# Configurar propiedades para la biblioteca
target_include_directories(ServerClient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ServerClient
    PUBLIC
        Qt6::Core
        Qt6::Network
)

# Habilitar MOC automáticamente para la biblioteca
set_target_properties(ServerClient PROPERTIES
    AUTOMOC ON
)

# Ejecutable de prueba completo
add_executable(TestMemoryProfiler
    test_complete_profiler.cpp
)

# Configurar propiedades para el ejecutable
target_include_directories(TestMemoryProfiler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TestMemoryProfiler
    PRIVATE
        ServerClient
        Qt6::Core
        Qt6::Network
)

# Habilitar MOC automáticamente para el ejecutable
set_target_properties(TestMemoryProfiler PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE FALSE
    DEBUG_POSTFIX "d"
    RELEASE_POSTFIX ""
)

# Mensaje de confirmación
message(STATUS "✅ TestMemoryProfiler configurado - Prueba completa del sistema")
message(STATUS "   Bibliotecas Qt6 encontradas: Core, Network")
message(STATUS "   Directorio de salida: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")