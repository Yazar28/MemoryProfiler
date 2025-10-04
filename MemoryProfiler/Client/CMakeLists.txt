cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Biblioteca del cliente
add_library(ServerClient STATIC
    ServerClient.cpp
    ServerClient.h
    profiler_structures.h
)

target_include_directories(ServerClient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ServerClient
    PUBLIC
        Qt6::Core
        Qt6::Network
)

# Habilitar MOC automáticamente
set_target_properties(ServerClient PROPERTIES
    AUTOMOC ON
)

# Prueba solo para GENERAL_METRICS
add_executable(TestMetricsOnly
    test_metrics_only.cpp
)

target_link_libraries(TestMetricsOnly
    PRIVATE
        ServerClient
        Qt6::Core
        Qt6::Network
)

# Configurar salida
set_target_properties(TestMetricsOnly PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"
    RELEASE_POSTFIX ""
)

message(STATUS "✅ Configuración completada: TestMetricsOnly - Solo envía GENERAL_METRICS")