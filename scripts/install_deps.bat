@echo off
rem vcpkg places static libraries under .lib
if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\yaml-cpp.lib" (
    echo libgit2 and yaml-cpp already installed.
    goto :eof
)
)

if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    echo libgit2 already installed.
    goto :eof
)

if exist vcpkg (
    echo Installing libgit2 and yaml-cpp via vcpkg...
    vcpkg\vcpkg install libgit2:x64-windows-static yaml-cpp:x64-windows-static
    goto :eof
)

echo libgit2 not found. Downloading vcpkg to install...
where git >nul 2>nul
if errorlevel 1 (
    echo Git is required to download vcpkg. Please install Git and retry.
    exit /b 1
)

git clone https://github.com/microsoft/vcpkg
cd vcpkg
call bootstrap-vcpkg.bat
cd ..
vcpkg\vcpkg install libgit2:x64-windows-static yaml-cpp:x64-windows-static

