cmake_minimum_required(VERSION 3.21)
project(MemoryProfiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuración preferida para MSVC
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compilador detectado: Microsoft Visual C++")
    set(COMPILER_TYPE "MSVC")
    # Optimizaciones para MSVC
    add_compile_options(/W4 /EHsc /MP)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Compilador detectado: GCC (MinGW)")
    set(COMPILER_TYPE "MINGW")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compilador detectado: Clang")
    set(COMPILER_TYPE "CLANG")
else()
    message(STATUS "Compilador detectado: ${CMAKE_CXX_COMPILER_ID}")
    set(COMPILER_TYPE "OTHER")
endif()

# Búsqueda flexible de Qt para MSVC principalmente
set(QT_PATHS_TO_TRY)

if(WIN32)
    # Prioridad a rutas de MSVC para Qt
    list(APPEND QT_PATHS_TO_TRY
        "$ENV{QT_DIR}"  # Primero intenta la variable de entorno
        "C:/Qt/6.9.2/msvc2022_64"
        "C:/Qt/6.9.2/msvc2019_64"
        "C:/Qt/6.9.1/msvc2022_64"
        "C:/Qt/6.9.1/msvc2019_64"
        "$ENV{USERPROFILE}/Qt/6.9.2/msvc2022_64"
        "$ENV{USERPROFILE}/Qt/6.9.2/msvc2019_64"
        "C:/Qt/6.9.0/msvc2022_64"
        "C:/Qt/6.9.0/msvc2019_64"
        # Rutas alternativas para MinGW (por si acaso)
        "C:/Qt/6.9.2/mingw_64"
        "C:/Qt/6.9.1/mingw_64"
    )
elseif(APPLE)
    list(APPEND QT_PATHS_TO_TRY
        "$ENV{QT_DIR}"
        "/opt/Qt/6.9.2/macos"
        "$ENV{HOME}/Qt/6.9.2/macos"
        "/Applications/Qt/6.9.2/macos"
        "/opt/Qt/6.9.1/macos"
        "$ENV{HOME}/Qt/6.9.1/macos"
    )
else()
    list(APPEND QT_PATHS_TO_TRY
        "$ENV{QT_DIR}"
        "/opt/Qt/6.9.2/gcc_64"
        "$ENV{HOME}/Qt/6.9.2/gcc_64"
        "/opt/Qt/6.9.1/gcc_64"
        "$ENV{HOME}/Qt/6.9.1/gcc_64"
    )
endif()

# Buscar Qt en todas las rutas posibles
set(QT_FOUND_PATH "")
foreach(QT_PATH IN LISTS QT_PATHS_TO_TRY)
    if(EXISTS "${QT_PATH}")
        set(QT_FOUND_PATH "${QT_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
        message(STATUS "Found Qt at: ${QT_PATH}")
        break()
    endif()
endforeach()

if(NOT QT_FOUND_PATH)
    message(WARNING "Qt no encontrado automáticamente. Puedes:")
    message(WARNING "1. Instalar Qt desde https://www.qt.io/download")
    message(WARNING "2. Definir la variable de entorno QT_DIR con la ruta a Qt")
    message(WARNING "3. Para MSVC, instalar Qt con el instalador online y seleccionar msvc2022_64")
    message(FATAL_ERROR "Qt es requerido para compilar este proyecto")
endif()

# Buscar el paquete Qt6
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Configuración de salida
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(lib)
add_subdirectory(gui)
add_subdirectory(tests)

# Mensajes informativos finales
message(STATUS "Qt6 encontrado en: ${Qt6_DIR}")
message(STATUS "Compilador: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Versión C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Archivos de build generados en: ${CMAKE_BINARY_DIR}")
message(STATUS "Para compilar: cmake --build ${CMAKE_BINARY_DIR} --config Release")