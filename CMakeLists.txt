cmake_minimum_required(VERSION 3.21)
project(MemoryProfiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/msvc2022_64")

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_subdirectory(lib)
add_subdirectory(gui)
add_subdirectory(tests)