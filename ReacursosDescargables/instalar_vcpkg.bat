::By Stuarth-195-Prof
::Git hub (https://github.com/Stuarth-195-Prof)
::No Repository
@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ================================
:: Script de instalación automática vcpkg
:: ================================

echo.
echo ===============================================
echo    INSTALADOR AUTOMÁTICO VCPKG Y LIBRERÍAS
echo ===============================================
echo.

:: Verificar si Git está instalado
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Git no está instalado o no está en el PATH
    echo Por favor, instala Git desde: https://git-scm.com/
    pause
    exit /b 1
)

:: Configurar directorio de vcpkg
set "VCPKG_ROOT=%CD%\vcpkg"

echo Directorio de instalación: %VCPKG_ROOT%
echo.

:: Clonar o actualizar vcpkg
if not exist "%VCPKG_ROOT%" (
    echo [1/5] Clonando vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_ROOT%"
    if !errorlevel! neq 0 (
        echo ERROR: Fallo al clonar vcpkg
        pause
        exit /b 1
    )
) else (
    echo [1/5] vcpkg ya existe, actualizando...
    cd "%VCPKG_ROOT%"
    git pull
    cd ..
)

:: Instalar vcpkg
echo.
echo [2/5] Instalando vcpkg...
cd "%VCPKG_ROOT%"
call bootstrap-vcpkg.bat
if !errorlevel! neq 0 (
    echo ERROR: Fallo en la instalación de vcpkg
    pause
    exit /b 1
)

:: Integración con Visual Studio
echo.
echo [3/5] Integrando vcpkg con Visual Studio...
call vcpkg integrate install

:: Instalar librerías
echo.
echo [4/5] Instalando librerías necesarias...
echo Esto puede tomar varios minutos...

call vcpkg install boost:x64-windows
call vcpkg install fmt:x64-windows
call vcpkg install spdlog:x64-windows
call vcpkg install gtest:x64-windows
call vcpkg install nlohmann-json:x64-windows

:: Completar
echo.
echo [5/5] Instalación completada!
echo.
echo =================================
echo RESULTADO DE LA INSTALACIÓN
echo =================================
echo vcpkg instalado en: %VCPKG_ROOT%
echo.
echo Librerías instaladas:
echo   - boost
echo   - fmt
echo   - spdlog
echo   - gtest
echo   - nlohmann-json
echo.
echo Para usar en Visual Studio:
echo 1. Abre Visual Studio
echo 2. Ve a Herramientas -> Opciones
echo 3. Busca "vcpkg"
echo 4. Configura la ruta: %VCPKG_ROOT%
echo.
echo Presiona cualquier tecla para cerrar...
pause >nul