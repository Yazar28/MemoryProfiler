# MemoryProfiler

Proyecto de profiling de memoria con interfaz Qt.

## Compilación

### Opción 1: Compilación simple (Recomendado)

```bash
# Configurar
cmake --preset=default

# Opcion 2: Compilar manual
cmake --build --preset=default
# Crear directorio de build
mkdir build
cd build

# Configurar
cmake ..

# Opcion 3 : Compiladores específicos
# Visual Studio 2022
cmake --build .
# Opción 3: Para IDEs específicos
# Visual Studio 2022
cmake --preset=vs2022

# Xcode (macOS)
cmake --preset=xcode

# Makefiles (Linux)
cmake --preset=make
```
