@echo off
setlocal enableextensions enabledelayedexpansion
echo Installing dependencies

rem Detect architecture for vcpkg triplet (x64 vs arm64)
set "TRIPLET=x64-windows-static"
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "TRIPLET=arm64-windows-static"
if /i "%PROCESSOR_ARCHITEW6432%"=="ARM64" set "TRIPLET=arm64-windows-static"

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
rem Install cpplint if missing, avoiding system-wide pip installs
where cpplint >nul 2>nul
if errorlevel 1 (
    rem Prefer winget/choco/pipx, fall back to local venv under .tools\venv
    where pipx >nul 2>nul
    if not errorlevel 1 (
        echo Installing cpplint via pipx
        pipx install --include-deps cpplint || goto :cpplint_venv
        goto :cpplint_done
    )

    rem Try winget to install pipx if available
    where winget >nul 2>nul
    if not errorlevel 1 (
        echo Installing pipx via winget
        winget install --id=pipxproject.pipx -e --silent || echo Skipping winget pipx install.
        where pipx >nul 2>nul && (
            echo Installing cpplint via pipx
            pipx install --include-deps cpplint && goto :cpplint_done
        )
    )

    rem Try Chocolatey to install pipx if available
    where choco >nul 2>nul
    if not errorlevel 1 (
        echo Installing pipx via choco
        choco install -y pipx || echo Skipping choco pipx install.
        refreshenv >nul 2>nul
        where pipx >nul 2>nul && (
            echo Installing cpplint via pipx
            pipx install --include-deps cpplint && goto :cpplint_done
        )
    )

    :cpplint_venv
    echo Setting up local venv for cpplint, no system pip changes
    set "TOOLS_DIR=.tools"
    set "VENV_DIR=%TOOLS_DIR%\venv"
    if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"
    where py >nul 2>nul
    if errorlevel 1 (
        where python >nul 2>nul || (
            echo Python not found. Please install Python 3 and re-run. & exit /b 1
        )
        python -m venv "%VENV_DIR%" || (
            echo Failed to create virtual environment. & exit /b 1
        )
        call "%VENV_DIR%\Scripts\activate.bat" && (
            python -m pip install --upgrade pip >nul 2>nul
            python -m pip install cpplint || echo Warning: cpplint installation in venv failed.
        )
    ) else (
        py -3 -m venv "%VENV_DIR%" || (
            echo Failed to create virtual environment. & exit /b 1
        )
        call "%VENV_DIR%\Scripts\activate.bat" && (
            py -3 -m pip install --upgrade pip >nul 2>nul
            py -3 -m pip install cpplint || echo Warning: cpplint installation in venv failed.
        )
    )
    set "PATH=%CD%\%VENV_DIR%\Scripts;%PATH%"
)

:cpplint_done
rem Check if each dependency is installed under vcpkg
set "LIBGIT2_INSTALLED=false"
set "YAMLCPP_INSTALLED=false"
set "JSON_INSTALLED=false"
set "ZLIB_INSTALLED=false"

if not defined VCPKG_ROOT (
    rem Try common locations
    if exist "%CD%\vcpkg" set "VCPKG_ROOT=%CD%\vcpkg"
    if not defined VCPKG_ROOT if exist "%LOCALAPPDATA%\vcpkg" set "VCPKG_ROOT=%LOCALAPPDATA%\vcpkg"
)
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\git2.lib" set "LIBGIT2_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\yaml-cpp.lib" set "YAMLCPP_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\include\nlohmann\json.hpp" set "JSON_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\zlib.lib" set "ZLIB_INSTALLED=true"
)

if "%LIBGIT2_INSTALLED%"=="true" if "%YAMLCPP_INSTALLED%"=="true" if "%JSON_INSTALLED%"=="true" if "%ZLIB_INSTALLED%"=="true" (
    echo libgit2, yaml-cpp, nlohmann-json and zlib already installed.
    goto :eof
)

set "PKGS="
if "%LIBGIT2_INSTALLED%"=="false" set "PKGS=!PKGS! libgit2:%TRIPLET%"
if "%YAMLCPP_INSTALLED%"=="false" set "PKGS=!PKGS! yaml-cpp:%TRIPLET%"
if "%JSON_INSTALLED%"=="false" set "PKGS=!PKGS! nlohmann-json:%TRIPLET%"
if "%ZLIB_INSTALLED%"=="false" set "PKGS=!PKGS! zlib:%TRIPLET%"

if exist vcpkg (
    echo Installing%PKGS% via vcpkg
    vcpkg\vcpkg install%PKGS%
    goto :eof
)

echo Dependencies not fully installed. Downloading vcpkg to install missing ones
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

endlocal


