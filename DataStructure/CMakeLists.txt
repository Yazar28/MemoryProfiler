# Mínima configuración de CMake para DataStructure
cmake_minimum_required(VERSION 3.10)
project(DataStructure)

# Incluir directorios de cabeceras
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Includes)

# Archivos fuente
set(SOURCES
	src/MyDoubleLinkedList.cpp
	src/MySimpleLinkedList.cpp
)

# Crear la biblioteca estática
add_library(DataStructure STATIC ${SOURCES})

# Ejecutable para pruebas unitarias
add_executable(TestDataStructure TestDataStructure.cpp)
target_link_libraries(TestDataStructure PRIVATE DataStructure)

# Opcional: instalar la biblioteca y headers
# install(TARGETS DataStructure DESTINATION lib)
# install(DIRECTORY Includes/ DESTINATION include)
