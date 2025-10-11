cmake_minimum_required(VERSION 3.21)
project(MemoryProfilerTests LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /EHsc /MP /Zc:__cplusplus)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Rutas de include para los 3 tests
set(TEST_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/../MemoryProfiler/src
    ${CMAKE_CURRENT_LIST_DIR}/../MemoryProfiler/Client
    ${CMAKE_CURRENT_LIST_DIR}/../MemoryProfiler/Include
)

# 1) basic_test
add_executable(basic_test
    basic_test.cpp
)
target_include_directories(basic_test PRIVATE ${TEST_INCLUDES})
target_link_libraries(basic_test PRIVATE MemoryProfiler Qt6::Core Qt6::Network)

# 2) test_memory_load
add_executable(test_memory_load
    test_memory_load.cpp
)
target_include_directories(test_memory_load PRIVATE ${TEST_INCLUDES})
target_link_libraries(test_memory_load PRIVATE MemoryProfiler Qt6::Core Qt6::Network)

# 3) test_edge_case   (ojo: tu archivo se llama en singular)
add_executable(test_edge_case
    test_edge_case.cpp
)
target_include_directories(test_edge_case PRIVATE ${TEST_INCLUDES})
target_link_libraries(test_edge_case PRIVATE MemoryProfiler Qt6::Core Qt6::Network)

# 4) test_from_fs
add_executable(test_from_fs
    test_from_fs.cpp
)
target_include_directories(test_from_fs PRIVATE ${TEST_INCLUDES})
target_link_libraries(test_from_fs PRIVATE MemoryProfiler Qt6::Core Qt6::Network)
