@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo Installing dependencies...

call :detect_triplet
call :ensure_tool_in_path "cmake" "C:\\Program Files\\CMake\\bin" "C:\\Program Files (x86)\\CMake\\bin"
call :ensure_tool_in_path "make" "%ProgramFiles%\\Git\\usr\\bin" "%ProgramFiles(x86)%\\GnuWin32\\bin"
call :ensure_cpplint
call :install_vcpkg_deps

goto :script_end

:detect_triplet
set "TRIPLET=x64-windows-static"
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "TRIPLET=arm64-windows-static"
if defined PROCESSOR_ARCHITEW6432 (
    if /i "%PROCESSOR_ARCHITEW6432%"=="ARM64" set "TRIPLET=arm64-windows-static"
)
exit /b 0

:ensure_tool_in_path
set "TOOL_NAME=%~1"
for %%P in (%*) do (
    if /i "%%~P"=="%TOOL_NAME%" (
        rem skip the tool name
    ) else (
        if exist "%%~P\%TOOL_NAME%.exe" (
            set "PATH=%%~P;%PATH%"
            goto :tool_check_done
        )
    )
)
:tool_check_done
where %TOOL_NAME% >nul 2>nul
exit /b 0

:ensure_cpplint
where cpplint >nul 2>nul && exit /b 0

where pipx >nul 2>nul
if not errorlevel 1 (
    echo Installing cpplint via pipx...
    if pipx install --include-deps cpplint >nul 2>nul exit /b 0
)

where winget >nul 2>nul
if not errorlevel 1 (
    echo Installing pipx via winget...
    winget install --id=pipxproject.pipx -e --silent >nul 2>nul
    where pipx >nul 2>nul && (
        echo Installing cpplint via pipx...
        if pipx install --include-deps cpplint >nul 2>nul exit /b 0
    )
)

where choco >nul 2>nul
if not errorlevel 1 (
    echo Installing pipx via choco...
    choco install -y pipx >nul 2>nul
    refreshenv >nul 2>nul
    where pipx >nul 2>nul && (
        echo Installing cpplint via pipx...
        if pipx install --include-deps cpplint >nul 2>nul exit /b 0
    )
)

echo Setting up local venv for cpplint (no system pip changes)...
set "TOOLS_DIR=.tools"
set "VENV_DIR=%TOOLS_DIR%\venv"
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"
set "PYTHON_EXE="
set "PYTHON_ARGS="
where py >nul 2>nul
if not errorlevel 1 (
    set "PYTHON_EXE=py"
    set "PYTHON_ARGS=-3"
    goto :found_python
)

where python >nul 2>nul || (
    echo Python not found. Please install Python 3 and re-run.
    exit /b 1
)
for /f "delims=" %%I in ('where python') do (
    set "PYTHON_EXE=%%I"
    goto :found_python
)

:found_python
if not defined PYTHON_EXE (
    echo Python not found. Please install Python 3 and re-run.
    exit /b 1
)

if exist "%VENV_DIR%" rd /s /q "%VENV_DIR%" >nul 2>nul
"%PYTHON_EXE%" %PYTHON_ARGS% -m venv "%VENV_DIR%" || (
    echo Failed to create virtual environment.
    exit /b 1
)
call "%VENV_DIR%\Scripts\activate.bat" || (
    echo Failed to activate virtual environment.
    exit /b 1
)
"%PYTHON_EXE%" %PYTHON_ARGS% -m pip install --upgrade pip >nul 2>nul
"%PYTHON_EXE%" %PYTHON_ARGS% -m pip install cpplint >nul 2>nul || echo Warning: cpplint installation in venv failed.
set "PATH=%CD%\%VENV_DIR%\Scripts;%PATH%"
exit /b 0

:install_vcpkg_deps
set "LIBGIT2_INSTALLED=false"
set "YAMLCPP_INSTALLED=false"
set "JSON_INSTALLED=false"
set "ZLIB_INSTALLED=false"

if not defined VCPKG_ROOT (
    if exist "%CD%\vcpkg" set "VCPKG_ROOT=%CD%\vcpkg"
    if not defined VCPKG_ROOT if exist "%LOCALAPPDATA%\vcpkg" set "VCPKG_ROOT=%LOCALAPPDATA%\vcpkg"
)

if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\git2.lib" set "LIBGIT2_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\yaml-cpp.lib" set "YAMLCPP_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\include\nlohmann\json.hpp" set "JSON_INSTALLED=true"
    if exist "%VCPKG_ROOT%\installed\%TRIPLET%\lib\zlib.lib" set "ZLIB_INSTALLED=true"
)

set "ALL_DEPS_INSTALLED=true"
if /i not "%LIBGIT2_INSTALLED%"=="true" set "ALL_DEPS_INSTALLED=false"
if /i not "%YAMLCPP_INSTALLED%"=="true" set "ALL_DEPS_INSTALLED=false"
if /i not "%JSON_INSTALLED%"=="true" set "ALL_DEPS_INSTALLED=false"
if /i not "%ZLIB_INSTALLED%"=="true" set "ALL_DEPS_INSTALLED=false"

if /i "%ALL_DEPS_INSTALLED%"=="true" (
    echo libgit2, yaml-cpp, nlohmann-json and zlib already installed.
    exit /b 0
)

set "PKGS="
call :append_pkg_if_missing "%LIBGIT2_INSTALLED%" "libgit2:%TRIPLET%"
call :append_pkg_if_missing "%YAMLCPP_INSTALLED%" "yaml-cpp:%TRIPLET%"
call :append_pkg_if_missing "%JSON_INSTALLED%" "nlohmann-json:%TRIPLET%"
call :append_pkg_if_missing "%ZLIB_INSTALLED%" "zlib:%TRIPLET%"

if exist vcpkg (
    echo Installing %PKGS% via vcpkg...
    vcpkg\vcpkg install %PKGS%
    exit /b %ERRORLEVEL%
)

echo Dependencies not fully installed. Downloading vcpkg to install missing ones...
where git >nul 2>nul || (
    echo Git is required to download vcpkg. Please install Git and retry.
    exit /b 1
)

git clone https://github.com/microsoft/vcpkg >nul
if errorlevel 1 (
    echo Failed to clone vcpkg.
    exit /b 1
)
cd vcpkg
call bootstrap-vcpkg.bat || (
    echo Failed to bootstrap vcpkg.
    cd ..
    exit /b 1
)
cd ..

echo Installing %PKGS% via vcpkg...
vcpkg\vcpkg install %PKGS%
exit /b %ERRORLEVEL%

:append_pkg_if_missing
if /i "%~1"=="true" exit /b 0
if defined PKGS (
    set "PKGS=%PKGS% %~2"
) else (
    set "PKGS=%~2"
)
exit /b 0

:script_end
endlocal
