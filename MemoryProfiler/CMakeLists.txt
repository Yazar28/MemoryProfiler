cmake_minimum_required(VERSION 3.21)
project(MemoryProfilerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /EHsc /MP /Zc:__cplusplus)
endif()

option(MT_DEBUG "Enable MemoryTracker debug logs" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Biblioteca principal
add_library(MemoryProfiler STATIC
    src/MemoryTracker.cpp
    src/MemoryOperators.cpp
)

target_include_directories(MemoryProfiler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(MemoryProfiler
    PUBLIC
        Qt6::Core
        Qt6::Network
)

set_target_properties(MemoryProfiler PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(MemoryProfiler PRIVATE /W4 /EHsc /permissive- /Zc:__cplusplus)
endif()

if(MT_DEBUG)
    target_compile_definitions(MemoryProfiler PRIVATE MT_DEBUG=1)
endif()

# Cliente
add_subdirectory(Client)

# Enlazar
target_link_libraries(MemoryProfiler PUBLIC ServerClient)