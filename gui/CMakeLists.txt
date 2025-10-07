#Gui
    cmake_minimum_required(VERSION 3.16)

project(MPGui VERSION 0.1 LANGUAGES CXX)

# Forzar uso de MSVC 2022 (Visual Studio 17)
if(NOT MSVC)
    message(FATAL_ERROR "❌ Este proyecto solo se puede compilar con MSVC (Visual Studio).")
endif()

# Verificar que la versión del compilador sea MSVC 2022 (19.30 - 19.49)
if (NOT (MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS 1950))
    message(FATAL_ERROR "❌ Se requiere Visual Studio 2022 (MSVC v19.3x - v19.4x). Versión detectada: ${MSVC_VERSION}")
else()
    message(STATUS "✅ Compilador MSVC 2022 detectado correctamente (versión: ${MSVC_VERSION})")
endif()

# Configuración específica para MSVC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_options(/W4 /WX- /Zc:__cplusplus /Zi)
add_link_options(/DEBUG)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar componentes de Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Network)

# Configurar variables de salida para MSVC
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Todos los archivos en la misma carpeta
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ListenLogic.cpp
    ListenLogic.h
    profiler_structures.h
)

# Para Qt6, usar qt_add_executable
qt_add_executable(MPGui
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    mainwindow.ui
)

target_link_libraries(MPGui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
)

set_target_properties(MPGui PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
    DEBUG_POSTFIX "d"
    RELEASE_POSTFIX ""
)

include(GNUInstallDirs)
install(TARGETS MPGui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(MPGui)
