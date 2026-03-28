
@echo off
:: ============================================================
::  build.bat  —  One-click build for GMap on Windows
::  Prerequisites: CMake 3.15+, MinGW-w64 or MSVC
:: ============================================================

echo.
echo  ╔══════════════════════════╗
echo  ║   Building GMap v1.0     ║
echo  ╚══════════════════════════╝
echo.

:: Create and enter the build directory
if not exist "build" mkdir build
cd build

:: Generate with MinGW Makefiles (change to "Visual Studio 17 2022" for MSVC)
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo  [ERROR] CMake configuration failed.
    echo  Make sure CMake and MinGW are installed and on your PATH.
    pause
    exit /b 1
)

:: Build
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

cd ..

echo.
echo  ✅  Build successful!
echo  Run:  build\gmap.exe
echo.
pause