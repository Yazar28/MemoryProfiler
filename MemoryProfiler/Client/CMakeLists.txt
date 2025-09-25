# Biblioteca del cliente (parte de MemoryProfiler)
add_library(ServerClient STATIC
    ServerClient.cpp
)

target_include_directories(ServerClient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Include
)

# Enlazar dependencias de Qt
target_link_libraries(ServerClient
    PUBLIC
        Qt6::Core
        Qt6::Network
)

set_target_properties(ServerClient PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Enlazar ServerClient con MemoryProfiler
target_link_libraries(MemoryProfiler
    PUBLIC
        ServerClient
)