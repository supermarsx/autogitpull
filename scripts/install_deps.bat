@echo off
setlocal enabledelayedexpansion

echo [autogitpull] Dependency install script is deprecated.
echo CMake (FetchContent) now builds third-party libs automatically.

where cmake >nul 2>nul
if errorlevel 1 (
  echo Please install CMake ^>= 3.20 and ensure it's on PATH.
  exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
  echo Please install Git and ensure it's on PATH.
  exit /b 1
)

echo You're good to go. Example:
echo   cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
echo   cmake --build build --config Release -j
endlocal
