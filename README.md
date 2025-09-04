# 🧠 MemoryProfiler

**MemoryProfiler** es una herramienta avanzada de profiling de memoria para aplicaciones C++ que permite analizar en tiempo real los patrones de uso de memoria, detectar memory leaks y visualizar el comportamiento de la memoria durante la ejecución de programas.

## ✨ Características Principales

### 🔍 Monitoreo en Tiempo Real

- Seguimiento continuo de asignaciones y liberaciones de memoria
- Métricas en tiempo real: uso actual, máximo histórico y tendencias
- Detección inmediata de fugas de memoria

### 📊 Visualización Intuitiva

- **Vista General**: Dashboard con métricas esenciales y línea de tiempo
- **Mapa de Memoria**: Representación visual de bloques de memoria con códigos de color
- **Análisis por Archivo**: Distribución de memoria por archivo fuente
- **Detector de Leaks**: Reportes detallados de fugas de memoria con gráficas

### 🛠️ Integración Transparente

- Biblioteca de instrumentalización que se integra fácilmente en proyectos C++ existentes
- Sobrecarga automática de operadores `new`, `delete`, `new[]` y `delete[]`
- Comunicación via socket con la interfaz gráfica

## 📋 Requisitos del Sistema

### Requisitos Mínimos

- **Sistema Operativo**: Windows 10/11, Linux (Ubuntu 18.04+), macOS (10.15+)
- **CMake**: versión 3.21 o superior
- **Compilador C++**: Compatible con C++17 (MSVC, GCC, Clang)
- **Qt**: versión 6.2 o superior
- **Memoria RAM**: 4 GB mínimo (8 GB recomendado)

### Dependencias

- Qt6 Widgets
- Biblioteca estándar de C++17
- Sockets para comunicación interprocesos

## 🚀 Compilación e Instalación

### Opción 1: Compilación Simple (Recomendado)

```bash
# Configurar el proyecto
cmake --preset=default

# Compilar
cmake --build --preset=default
```

### Opción 2: Compilación Manual

```bash
# Crear directorio de build
mkdir build
cd build

# Configurar
cmake ..

# Compilar
cmake --build .
```

### Opción 3: Para IDEs Específicos

```bash
# Visual Studio 2022
cmake --preset=vs2022

# Xcode (macOS)
cmake --preset=xcode

# Makefiles (Linux)
cmake --preset=make
```

## 🎯 Uso del MemoryProfiler

### Integración en Proyectos Existente

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

## 📁 Estructura del Proyecto

```
MemoryProfiler/
├── lib/                 # Biblioteca de instrumentalización
├── gui/                 # Interfaz gráfica Qt
├── tests/               # Programas de prueba
├── CMakeLists.txt       # Configuración principal de CMake
├── CMakePresets.json    # Presets de compilación
└── README.md           # Este archivo
```

## 🎨 Interfaz Gráfica

### Pestaña de Vista General

- **Métricas en Tiempo Real**: Uso actual, asignaciones activas, memory leaks
- **Línea de Temporal**: Evolución del uso de memoria durante la ejecución
- **Top 3 Archivos**: Archivos con mayor asignación de memoria

### Mapa de Memoria

- Visualización de todos los bloques de memoria asignados
- Códigos de color para diferentes estados (activo, liberado, fugado)
- Información detallada al hacer hover sobre bloques

### Análisis por Archivo Fuente

- Distribución de memoria por archivo .cpp/.h
- Conteo de asignaciones y memoria total por archivo

### Detector de Memory Leaks

- Reporte de fugas detectadas
- Gráficas de distribución y temporal de leaks
- Identificación de archivos con mayor frecuencia de leaks

## 🔧 Configuración Avanzada

### Variables de Entorno Opcionales

```bash
# Especificar ruta personalizada de Qt
export QT_DIR=/ruta/personalizada/qt  # Linux/macOS
set QT_DIR=C:\ruta\personalizada\qt   # Windows

# Especificar tipo de build
export BUILD_TYPE=Debug    # o Release
```

### Personalización de CMake

```bash
# Opciones de configuración adicionales
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQT_DIR=/ruta/qt
```

## 🤝 Contribución

Las contribuciones son bienvenidas. Por favor, asegúrese de:

1. Seguir los estándares de código existentes
2. Probar cambios en todas las plataformas compatibles
3. Actualizar la documentación correspondiente
ecto está licenciado bajo la Licencia MIT. Vea el archivo LICENSE para más detalles.

## 🆘 Soporte

Para problemas técnicos o preguntas:


1. Abra un issue en el repositorio GitHub
2. Contacte al equipo de desarrollo

---

**MemoryProfiler** - Porque cada byte cuenta 🧠⚡
