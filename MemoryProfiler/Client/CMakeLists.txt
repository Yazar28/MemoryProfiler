cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Biblioteca del cliente
add_library(ServerClient STATIC
    ServerClient.cpp
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

# Ejecutable de prueba específico para GENERAL_METRICS
add_executable(TestClientGeneralMetrics
    test_client.cpp
)

target_link_libraries(TestClientGeneralMetrics
    PRIVATE
        ServerClient
        Qt6::Core
        Qt6::Network
)
