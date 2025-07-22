@echo off
rem *********************************************************************
rem  AutoGitPull - release build script (Windows / MinGW‑w64)
rem *********************************************************************
rem  Builds a fully static release binary, compiling third‑party libs on demand.
rem  Invoke from anywhere; the script locates the project root automatically.
rem *********************************************************************

setlocal EnableDelayedExpansion

rem --------------------------------------------------------------------
rem Resolve project root directory                                       
rem --------------------------------------------------------------------
for %%I in ("%~dp0..") do set "ROOT_DIR=%%~fI"
if not defined ROOT_DIR (
    echo [ERROR] Unable to determine project root. Aborting.
    exit /b 1
)
cd /d "%ROOT_DIR%" || (
    echo [ERROR] Could not change directory to project root: %ROOT_DIR%
    exit /b 1
)

echo ============================================================
echo  AutoGitPull - Release Build Started
echo  Project root: %ROOT_DIR%
echo ============================================================
echo.
echo [1/4] Checking toolchain

rem --------------------------------------------------------------------
rem Ensure MinGW‑w64 toolchain is present                                
rem --------------------------------------------------------------------
echo Checking for g++ in PATH...
where g++ >nul 2>nul
if errorlevel 1 (
    echo [WARN] g++ not found.
    if defined ChocolateyInstall (
        echo Installing MinGW‑w64 via Chocolatey...
        choco install -y mingw || exit /b 1
        echo MinGW‑w64 installed.
    ) else (
        echo [ERROR] Chocolatey not detected. Install MinGW‑w64 and add g++ to PATH.
        exit /b 1
    )
) else (
    for /f "tokens=2 delims== " %%v in ('g++ -v 2^>^&1 ^| findstr "gcc version"') do set "GCC_VERSION=%%v"
    echo g++ found, version !GCC_VERSION!.
)

rem --------------------------------------------------------------------
rem Dependency locations                                                 
rem --------------------------------------------------------------------
set "LIBGIT2_INC=%ROOT_DIR%\libs\libgit2\libgit2_install\include"
set "LIBGIT2_LIB=%ROOT_DIR%\libs\libgit2\libgit2_install\lib"
set "YAMLCPP_INC=%ROOT_DIR%\libs\yaml-cpp\yaml-cpp_install\include"
set "YAMLCPP_LIB=%ROOT_DIR%\libs\yaml-cpp\yaml-cpp_install\lib"
set "JSON_INC=%ROOT_DIR%\libs\nlohmann-json\single_include"

rem --------------------------------------------------------------------
rem Build / fetch third‑party libs if missing                             
rem --------------------------------------------------------------------
echo.
echo ============================================================
echo  [2/4] Checking third‑party dependencies
echo ============================================================
if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call "%ROOT_DIR%\scripts\install_libgit2_mingw.bat" || exit /b 1
) else echo libgit2 detected.

if not exist "%YAMLCPP_LIB%\libyaml-cpp.a" (
    call "%ROOT_DIR%\scripts\install_yamlcpp_mingw.bat" || exit /b 1
) else echo yaml‑cpp detected.

if not exist "%JSON_INC%\nlohmann\json.hpp" (
    call "%ROOT_DIR%\scripts\install_nlohmann_json.bat" || exit /b 1
) else echo nlohmann‑json detected.

rem --------------------------------------------------------------------
rem Output & intermediate directories                                    
rem --------------------------------------------------------------------
set "BUILD_DIR=%ROOT_DIR%\build"
set "OBJ_DIR=%BUILD_DIR%\obj"
set "DIST_DIR=%ROOT_DIR%\dist"
if not exist "%OBJ_DIR%"  mkdir "%OBJ_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

echo.
echo ============================================================
echo  [3/4] Resource compilation
echo ============================================================
windres -I "%ROOT_DIR%\include" -F pe-x86-64 -O coff -i "%ROOT_DIR%\src\version.rc" -o "%OBJ_DIR%\version.o" || exit /b 1

if not exist "%ROOT_DIR%\graphics\icon.ico" (
    call "%ROOT_DIR%\scripts\generate_icons.bat" || exit /b 1
)
windres -I "%ROOT_DIR%\graphics" -F pe-x86-64 -O coff -i "%ROOT_DIR%\graphics\icon.rc" -o "%OBJ_DIR%\icon.o" || exit /b 1
set "RES_OBJS=%OBJ_DIR%\version.o %OBJ_DIR%\icon.o"

rem --------------------------------------------------------------------
rem Compose source list (no quotes to avoid ld issues)                    
rem --------------------------------------------------------------------
set "SRCS="
for %%F in ( 
    "%ROOT_DIR%\src\autogitpull.cpp"
    "%ROOT_DIR%\src\scanner.cpp"
    "%ROOT_DIR%\src\ui_loop.cpp"
    "%ROOT_DIR%\src\git_utils.cpp"
    "%ROOT_DIR%\src\tui.cpp" 
    "%ROOT_DIR%\src\logger.cpp" 
    "%ROOT_DIR%\src\resource_utils.cpp" 
    "%ROOT_DIR%\src\system_utils.cpp" 
    "%ROOT_DIR%\src\time_utils.cpp" 
    "%ROOT_DIR%\src\config_utils.cpp" 
    "%ROOT_DIR%\src\debug_utils.cpp" 
    "%ROOT_DIR%\src\options.cpp" 
    "%ROOT_DIR%\src\parse_utils.cpp"
    "%ROOT_DIR%\src\lock_utils.cpp"
    "%ROOT_DIR%\src\process_monitor.cpp"
    "%ROOT_DIR%\src\windows_service.cpp"
) do set "SRCS=!SRCS! %%~F"

rem --------------------------------------------------------------------
rem Compiler & linker flags                                               
rem --------------------------------------------------------------------
if not defined CXXFLAGS (
    set "CXXFLAGS=-std=c++20 -O2 -pipe -static -static-libgcc -static-libstdc++ -DYAML_CPP_STATIC_DEFINE"
)
set "INCLUDE_FLAGS=-I%LIBGIT2_INC% -I%YAMLCPP_INC% -I%JSON_INC% -I%ROOT_DIR%\include"
set "LDFLAGS=%LIBGIT2_LIB%\libgit2.a %YAMLCPP_LIB%\libyaml-cpp.a -lssh2 -lz -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi -ladvapi32"

rem --------------------------------------------------------------------
rem Build                                                                 
rem --------------------------------------------------------------------
echo.
echo ============================================================
echo  [4/4] Compiling and linking
echo ============================================================
echo This may take a moment...

g++ %CXXFLAGS% %INCLUDE_FLAGS% !SRCS! %RES_OBJS% %LDFLAGS% -o "%DIST_DIR%\autogitpull.exe"
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo Build completed successfully. Binary: %DIST_DIR%\autogitpull.exe

endlocal
