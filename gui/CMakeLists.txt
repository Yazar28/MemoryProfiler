cmake_minimum_required(VERSION 3.16)

project(Prueva3 VERSION 0.1 LANGUAGES CXX)

# Configuración específica para MSVC
if(MSVC)
    # Configurar las opciones de tiempo de ejecución de MSVC
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Deshabilitar advertencias específicas si es necesario
    add_compile_options(/W4 /WX-)
    
    # Configurar estándar de C++ para MSVC
    add_compile_options(/Zc:__cplusplus)
    
    # Habilitar generación de información de depuración
    add_compile_options(/Zi)
    
    # Configurar opciones de enlace
    add_link_options(/DEBUG)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar componentes de Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Network)

# Configurar variables de salida para MSVC
if(MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# Todos los archivos en la misma carpeta
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    LisenLogic.cpp
)

# Para Qt6, usar qt_add_executable
qt_add_executable(Prueva3
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    mainwindow.ui  # Incluir el archivo UI directamente
)

target_link_libraries(Prueva3 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
)

# Configurar propiedades del objetivo
set_target_properties(Prueva3 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE  # Deshabilitar bundle para macOS si no es necesario
)

# Para MSVC, configurar el sufijo de salida
if(MSVC)
    set_target_properties(Prueva3 PROPERTIES
        DEBUG_POSTFIX "d"
        RELEASE_POSTFIX ""
    )
endif()

# Configuración de instalación (opcional)
include(GNUInstallDirs)
install(TARGETS Prueva3
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Finalización para Qt6
qt_finalize_executable(Prueva3)
