# README — instalador vcpkg

**Propósito:**
Este archivo README explica qué hace el script `instalar_vcpkg.bat` y cómo usarlo de forma segura. El `.bat` está pensado principalmente para **descargar e instalar recursos (bibliotecas) necesarios para un proyecto en C++** usando vcpkg.

---

## Qué hace el script

- Clona/descarga `vcpkg` (si no existe).
- Ejecuta el _bootstrap_ de `vcpkg` para preparar la herramienta en Windows.
- Instala una lista de paquetes (ej.: `boost`, `fmt`, `spdlog`, `gtest`, `nlohmann-json`).
- Muestra instrucciones rápidas para integrar `vcpkg` con Visual Studio y CMake.

> **Nota:** la lista de paquetes puede variar según la versión del script. Verifica el contenido del `.bat` si necesitas confirmar qué se descargará.

---

## Requisitos previos

- Windows (CMD / PowerShell).
- Git disponible en la ruta (si el script hace `git clone`).
- Visual Studio o entorno CMake si piensas integrar `vcpkg` con el IDE.
- Espacio en disco suficiente (las compilaciones pueden ocupar cientos de MBs o más).
- Conexión a internet (se descargarán fuentes/binaries).

---

## Advertencias importantes (léelas antes de ejecutar)

- **No ejecutes este script si no necesitas las librerías que descarga.** Descarga y compila dependencias que pueden ocupar espacio y consumir ancho de banda.
- **No ejecutes el script en `C:\` (la raíz del disco) ni en carpetas del sistema.** Ejecutarlo en la raíz puede crear muchos archivos y permisos inesperados. Usa una carpeta específica para herramientas, por ejemplo: `C:\dev\tools\vcpkg` o `D:\shared\vcpkg`.
- **No coloques la carpeta de `vcpkg` ni los artefactos binarios en el repositorio del código fuente** (no subir los binarios ni la carpeta `installed` al control de versiones). En su lugar:

  - Coloca el instalador `.bat` o un pequeño `README`/manifest en el repositorio para que otros sepan cómo obtener las dependencias.
  - Los recursos descargados deben estar en una ubicación externa al repo (por ejemplo `C:\dev\third_party\vcpkg` o una carpeta compartida en red) y agregar esa ruta en la configuración del IDE.

- **Revisa el `.bat` antes de ejecutarlo.** Asegúrate de entender cada comando (especialmente `rm`, `del`, `rd`, ejecución como admin o cambios de PATH) y de que proviene de una fuente confiable.

---

## Dónde guardar/ejecutar el instalador (recomendación)

- Crear una carpeta dedicada fuera del repositorio, por ejemplo:

  - `C:\dev\vcpkg` o `D:\tools\vcpkg`
  - O una ruta de red compartida accesible por el equipo de desarrollo: `\\servidor\libs\vcpkg`

- Los IDEs (Visual Studio, CLion con CMake, etc.) deben apuntar a `VCPKG_ROOT` o al archivo `vcpkg.cmake` que proporciona la integración.
- Si trabajas en equipo, **incluye en el repo solamente**:

  - El script `.bat` o un `README.md` con instrucciones de instalación.
  - Opcional: un manifest `vcpkg.json` para fijar versiones de paquetes (esto sí puede ir en el repo, porque describe dependencias, pero no incluye binarios).

---

## Cómo usarlo (ejemplo)

1. Abrir una ventana de CMD (no es necesario ejecutarlo como administrador salvo que el script lo pida explícitamente).
2. Navegar a la carpeta donde guardaste `instalar_vcpkg.bat`.
3. Ejecutar:

```bat
instalar_vcpkg.bat
```

4. Seguir las instrucciones en pantalla. Cuando termine, configura tu IDE para que apunte a la ruta de `vcpkg`.

### Configuración con CMake

Añade al configurar CMake:

```bash
-DCMAKE_TOOLCHAIN_FILE="C:/ruta/a/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Integración con Visual Studio

Desde la línea de comandos dentro de la carpeta de vcpkg puedes ejecutar:

```bat
vcpkg integrate install
```

o en Visual Studio ir a **Herramientas → Opciones → vcpkg** y configurar la ruta `VCPKG_ROOT` (si tu IDE lo soporta).

---

## Paquetes incluidos (resumen)

El script instala (según la versión actual del `.bat`) al menos los siguientes paquetes:

- boost
- fmt
- spdlog
- gtest
- nlohmann-json

**Si necesitas otros paquetes**, edita el `.bat` o usa `vcpkg install <paquete>` manualmente.

---

## Seguridad y mantenimiento

- Mantén el `.bat` en control de versiones (es pequeño) y **añade la carpeta de instalación de vcpkg a `.gitignore`** si por alguna razón se encuentra dentro del árbol del repo.
- Revisa periódicamente las versiones de las bibliotecas y considera usar un `vcpkg.json` (manifiesto) en el repo para reproducibilidad.
- Si trabajas en entornos de CI, instala `vcpkg` dentro del entorno de build en lugar de subir binarios al repo.

---

## Solución de problemas común

- **Errores por falta de espacio:** libera espacio o instala en otra unidad.
- **Problemas de permisos:** evita ejecutar como admin si no es necesario; si falla por permisos, revisa el mensaje y ejecuta como administrador solo si es imprescindible.
- **Proxy / red:** configura Git/cURL/Windows para usar el proxy si tu red lo requiere.

---

## Licencia y contacto

Este script y README son para uso interno del proyecto. Si necesitas ayuda para adaptarlo a tu flujo de trabajo (p. ej. CI o rutas compartidas), contáctame y te puedo ayudar a ajustar los pasos.

---

> **Resumen final:** el `.bat` automatiza la instalación de `vcpkg` y varias bibliotecas para proyectos C++. **No lo ejecutes en la raíz del disco ni en repositorios** con archivos de código; mejor colócalo en una ruta dedicada donde tu IDE pueda leer la configuración (`VCPKG_ROOT` / `vcpkg.cmake`). Revisa siempre el contenido del `.bat` antes de ejecutar.
