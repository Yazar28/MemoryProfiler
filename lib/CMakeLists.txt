add_library(memorytracker
    MemoryTracker.cpp
    AllocationInfo.cpp
)
target_include_directories(memorytracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
