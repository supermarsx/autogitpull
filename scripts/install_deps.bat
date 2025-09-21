@echo off
echo Installing dependencies...
rem Auto-detect cmake and make if they are not in PATH
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\CMake\bin;%PATH%"
    ) else if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files (x86)\CMake\bin;%PATH%"
    )
)
where make >nul 2>nul
if errorlevel 1 (
    if exist "%ProgramFiles%\Git\usr\bin\make.exe" (
        set "PATH=%ProgramFiles%\Git\usr\bin;%PATH%"
    ) else if exist "%ProgramFiles(x86)%\GnuWin32\bin\make.exe" (
        set "PATH=%ProgramFiles(x86)%\GnuWin32\bin;%PATH%"
    )
)
rem Install cpplint if missing
where cpplint >nul 2>nul
if errorlevel 1 (
    pip install cpplint
)
rem Check if each dependency is installed under vcpkg
set "LIBGIT2_INSTALLED=false"
set "YAMLCPP_INSTALLED=false"
set "JSON_INSTALLED=false"
set "ZLIB_INSTALLED=false"

if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    set "LIBGIT2_INSTALLED=true"
)
if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\yaml-cpp.lib" (
    set "YAMLCPP_INSTALLED=true"
)
if exist "%VCPKG_ROOT%\installed\x64-windows-static\include\nlohmann\json.hpp" (
    set "JSON_INSTALLED=true"
)
if exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\zlib.lib" (
    set "ZLIB_INSTALLED=true"
)

if "%LIBGIT2_INSTALLED%"=="true" if "%YAMLCPP_INSTALLED%"=="true" if "%JSON_INSTALLED%"=="true" if "%ZLIB_INSTALLED%"=="true" (
    echo libgit2, yaml-cpp, nlohmann-json and zlib already installed.
    goto :eof
)

set "PKGS="
if "%LIBGIT2_INSTALLED%"=="false" set "PKGS=%PKGS% libgit2:x64-windows-static"
if "%YAMLCPP_INSTALLED%"=="false" set "PKGS=%PKGS% yaml-cpp:x64-windows-static"
if "%JSON_INSTALLED%"=="false" set "PKGS=%PKGS% nlohmann-json"
if "%ZLIB_INSTALLED%"=="false" set "PKGS=%PKGS% zlib"

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


