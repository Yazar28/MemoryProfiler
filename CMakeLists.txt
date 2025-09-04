cmake_minimum_required(VERSION 3.21)
project(MemoryProfiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detección automática de Qt sin rutas fijas
if(DEFINED ENV{QT_DIR})
    message(STATUS "Using QT_DIR from environment: $ENV{QT_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
else()
    # Rutas comunes por plataforma (solo como fallback)
    if(WIN32)
        set(DEFAULT_QT_PATHS 
            "C:/Qt/6.9.2/msvc2022_64"
            "C:/Qt/6.9.1/msvc2022_64"
            "$ENV{USERPROFILE}/Qt/6.9.2/msvc2022_64"
        )
    elseif(APPLE)
        set(DEFAULT_QT_PATHS
            "/opt/Qt/6.9.2/macos"
            "$ENV{HOME}/Qt/6.9.2/macos"
            "/Applications/Qt/6.9.2/macos"
        )
    else()
        set(DEFAULT_QT_PATHS
            "/opt/Qt/6.9.2/gcc_64"
            "$ENV{HOME}/Qt/6.9.2/gcc_64"
        )
    endif()
    
    # Buscar Qt en rutas comunes
    foreach(QT_PATH IN LISTS DEFAULT_QT_PATHS)
        if(EXISTS "${QT_PATH}")
            list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Found Qt at: ${QT_PATH}")
            break()
        endif()
    endforeach()
endif()

# Buscar Qt6
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Configuración de salida
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(lib)
add_subdirectory(gui)
add_subdirectory(tests)

# Mensaje final con información útil
message(STATUS "Qt6 found at: ${Qt6_DIR}")
message(STATUS "Build files generated in: ${CMAKE_BINARY_DIR}")
message(STATUS "To build: cmake --build ${CMAKE_BINARY_DIR}")