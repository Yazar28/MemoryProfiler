# 🧩 Compilar y ejecutar módulos o tests individuales

Puedes compilar y ejecutar partes específicas del proyecto (por ejemplo, un test de una estructura de datos) siguiendo estos pasos:

### Ejemplo: Compilar y ejecutar solo el test de lista simple

1. **Configura el proyecto (solo la primera vez o si cambias CMake):**

```bash
cmake -S . -B build
```

2. **Compila solo el ejecutable de test:**

```bash
cmake --build build --target TestDataStructure
```

3. **Ejecuta el binario generado:**

```bash
./build/bin/Debug/TestDataStructure.exe
```

(O busca el ejecutable en `build/bin/` o la carpeta que corresponda según tu configuración y sistema operativo.)

---

### Ejemplo: Agregar un nuevo test o ejecutable

Supón que creas un archivo `TestMiEstructura.cpp` en `DataStructure/` para probar otra estructura:

1. **Edita `DataStructure/CMakeLists.txt` y agrega:**

```cmake
add_executable(TestMiEstructura TestMiEstructura.cpp)
target_link_libraries(TestMiEstructura PRIVATE DataStructure)
```

2. **Regenera los archivos de build:**

```bash
cmake -S . -B build
```

3. **Compila solo el nuevo test:**

```bash
cmake --build build --target TestMiEstructura
```

4. **Ejecuta el nuevo test:**

```bash
./build/bin/Debug/TestMiEstructura.exe
```

---

### Ejemplo: Agregar un nuevo archivo fuente a la biblioteca

1. Crea tu archivo, por ejemplo `src/MiNuevaEstructura.cpp`.
2. Edita la lista de fuentes en `DataStructure/CMakeLists.txt`:

```cmake
set(SOURCES
   src/MyDoubleLinkedList.cpp
   src/MySimpleLinkedList.cpp
   src/MiNuevaEstructura.cpp
)
```

3. Regenera y recompila:

```bash
cmake -S . -B build
cmake --build build
```

---

### Notas importantes

- **Siempre que cambies el `CMakeLists.txt` (agregues tests, fuentes, etc.), ejecuta `cmake -S . -B build` antes de compilar.**
- Puedes compilar todos los ejecutables y bibliotecas con:
  ```bash
  cmake --build build
  ```
- Si tienes dudas sobre el nombre del ejecutable, revisa la carpeta `build/bin/Debug/` o el output de la compilación.

Esto te permite trabajar y probar módulos o tests de forma independiente, sin recompilar todo el proyecto cada vez.

# MemoryProfiler

<div align="center">
  <!-- Espacio para GIF de demostración -->
  <img src="./.github/demo.gif" alt="Demonstración del MemoryProfiler" width="600"/>
  
  &#xa0;
</div>

<h1 align="center">🧠 MemoryProfiler</h1>

<p align="center">
  <img alt="Lenguaje principal" src="https://img.shields.io/badge/C++-17-blue.svg?style=for-the-badge">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6.2-green.svg?style=for-the-badge">
  <img alt="CMake" src="https://img.shields.io/badge/CMake-3.21+-brightgreen.svg?style=for-the-badge">
  <img alt="Plataformas" src="https://img.shields.io/badge/Plataformas-Windows%20|%20Linux%20|%20macOS-lightgrey.svg?style=for-the-badge">
</p>

## 📖 Acerca del proyecto

**MemoryProfiler** es una herramienta avanzada de análisis de memoria para aplicaciones C++ que permite monitorear en tiempo real el uso de memoria, detectar memory leaks y visualizar el comportamiento de la memoria durante la ejecución de programas.

## ✨ Características

- 🔍 **Monitoreo en tiempo real** de asignaciones y liberaciones de memoria
- 📊 **Interfaz gráfica intuitiva** con múltiples pestañas de análisis
- 🚨 **Detección automática** de memory leaks y fugas de memoria
- 📈 **Visualización gráfica** del mapa de memoria y tendencias temporales
- 📁 **Análisis por archivo fuente** con distribución detallada
- 🔌 **Comunicación por sockets** entre la biblioteca y la interfaz

## 🛠️ Tecnologías utilizadas

- **C++17** - Lenguaje de programación principal
- **Qt6** - Framework para la interfaz gráfica
- **CMake** - Sistema de construcción multiplataforma
- **Git** - Control de versiones

## 📋 Requisitos del sistema

### Requisitos mínimos

- **Sistema operativo**: Windows 10/11, Linux (Ubuntu 18.04+), macOS (10.15+)
- **CMake**: versión 3.21 o superior
- **Compilador C++**: Compatible con C++17 (MSVC, GCC, Clang)
- **Qt**: versión 6.2 o superior
- **Memoria RAM**: 4 GB mínimo (8 GB recomendado)

## 🚀 Comenzando

### Clonar el repositorio

```bash
git clone[ https://github.com/tu_usuario/memoryprofiler](https://github.com/Yazar28/MemoryProfiler.git)
cd memoryprofiler
```

### Compilación (método recomendado)

```bash
# Configurar el proyecto
cmake --preset=default

# Compilar
cmake --build --preset=default
```

### Compilación manual

```bash
# Crear directorio de construcción
mkdir build
cd build

# Configurar
cmake ..

# Compilar
cmake --build .
```

### Compilación para IDEs específicos

```bash
# Visual Studio 2022 (Windows)
cmake --preset=vs2022

# Xcode (macOS)
cmake --preset=xcode

# Makefiles (Linux)
cmake --preset=make
```

## 📁 Estructura del proyecto

```
MemoryProfiler/
├── lib/                 # Biblioteca de instrumentalización
├── gui/                 # Interfaz gráfica Qt
├── tests/               # Programas de prueba
├── CMakeLists.txt       # Configuración principal de CMake
├── CMakePresets.json    # Presets de compilación
└── README.md           # Este archivo
```

## 🎯 Uso del MemoryProfiler

### Integración en proyectos existentes

```cpp
#include "MemoryProfiler.h"

// El profiler se activa automáticamente al incluir el header
int main() {
    // Tu código normal...
    int* myArray = new int[100]; // Monitoreado automáticamente
    // ...
    delete[] myArray; // Registrado automáticamente
    return 0;
}
```

### Ejecución

1. Compile su aplicación con la biblioteca de instrumentalización
2. Ejecute la interfaz gráfica del MemoryProfiler
3. Inicie su aplicación instrumentada
4. Observe en tiempo real el comportamiento de la memoria

## 📊 Funcionalidades de la interfaz

### Pestaña de Vista General

- Métricas en tiempo real: uso actual, asignaciones activas, memory leaks
- Línea temporal: evolución del uso de memoria durante la ejecución
- Top 3 archivos: archivos con mayor asignación de memoria

### Mapa de Memoria

- Visualización de todos los bloques de memoria asignados
- Códigos de color para diferentes estados (activo, liberado, fugado)
- Información detallada al pasar el cursor sobre bloques

### Análisis por Archivo Fuente

- Distribución de memoria por archivo .cpp/.h
- Conteo de asignaciones y memoria total por archivo

### Detector de Memory Leaks

- Reporte de fugas detectadas
- Gráficas de distribución y temporal de leaks
- Identificación de archivos con mayor frecuencia de leaks

## 🔧 Configuración avanzada

### Variables de entorno opcionales

```bash
# Especificar ruta personalizada de Qt
export QT_DIR=/ruta/personalizada/qt  # Linux/macOS
set QT_DIR=C:\ruta\personalizada\qt   # Windows

# Especificar tipo de construcción
export BUILD_TYPE=Debug    # o Release
```

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor, asegúrate de:

1. Seguir los estándares de código existentes
2. Probar cambios en todas las plataformas compatibles
3. Actualizar la documentación correspondiente

## 🆘 Soporte

Para problemas técnicos o preguntas:

1. Abre un issue en el [repositorio de GitHub](https://github.com/tu_usuario/memoryprofiler/issues)
2. Contacta al equipo de desarrollo

&#xa0;

<a href="#top">Volver al inicio</a>
