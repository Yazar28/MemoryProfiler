cmake_minimum_required(VERSION 3.21)

add_executable(test_tracker
    test_tracker.cpp
)

target_link_libraries(test_tracker PRIVATE MemoryProfiler)

if(MSVC)
  target_compile_options(test_tracker PRIVATE /W4 /EHsc /permissive- /Zc:__cplusplus)
endif()
