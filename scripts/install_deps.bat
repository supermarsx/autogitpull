@echo off
echo Installing dependencies...
rem Install cpplint if missing
where cpplint >nul 2>nul
if errorlevel 1 (
    pip install cpplint
)
rem Check if each dependency is installed under vcpkg
set "LIBGIT2_INSTALLED=false"
set "YAMLCPP_INSTALLED=false"
set "JSON_INSTALLED=false"

if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    set "LIBGIT2_INSTALLED=true"
)
if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\yaml-cpp.lib" (
    set "YAMLCPP_INSTALLED=true"
)
if exist "%VCPKG_ROOT%\installed\x64-windows-static\include\nlohmann\json.hpp" (
    set "JSON_INSTALLED=true"
)

if "%LIBGIT2_INSTALLED%"=="true" if "%YAMLCPP_INSTALLED%"=="true" if "%JSON_INSTALLED%"=="true" (
    echo libgit2, yaml-cpp and nlohmann-json already installed.
    goto :eof
)

set "PKGS="
if "%LIBGIT2_INSTALLED%"=="false" set "PKGS=%PKGS% libgit2:x64-windows-static"
if "%YAMLCPP_INSTALLED%"=="false" set "PKGS=%PKGS% yaml-cpp:x64-windows-static"
if "%JSON_INSTALLED%"=="false" set "PKGS=%PKGS% nlohmann-json"

if exist vcpkg (
    echo Installing%PKGS% via vcpkg...
    vcpkg\vcpkg install%PKGS%
    goto :eof
)

echo Dependencies not fully installed. Downloading vcpkg to install missing ones...
where git >nul 2>nul
if errorlevel 1 (
    echo Git is required to download vcpkg. Please install Git and retry.
    exit /b 1
)

git clone https://github.com/microsoft/vcpkg
cd vcpkg
call bootstrap-vcpkg.bat
cd ..
vcpkg\vcpkg install%PKGS%


