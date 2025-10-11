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

# Qt para la GUI
set(Qt6_DIR "C:/Qt/6.9.2/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(MemoryProfiler)
add_subdirectory(gui)
add_subdirectory(tests)

message(STATUS "Qt6: ${Qt6_DIR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build dir: ${CMAKE_BINARY_DIR}")
