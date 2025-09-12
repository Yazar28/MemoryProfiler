cmake_minimum_required(VERSION 3.21)

add_library(MemoryProfiler STATIC
    src/MemoryTracker.cpp
    src/MemoryOperators.cpp
    # src/SocketClient.cpp     # déjalo si existe; si no, bórralo
)

target_include_directories(MemoryProfiler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
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
