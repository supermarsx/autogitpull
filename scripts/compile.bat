@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
cd /d "%ROOT_DIR%"

where g++ >nul 2>nul
if errorlevel 1 (
    echo MinGW g++ not found. Attempting to install...
    if defined ChocolateyInstall (
        choco install -y mingw
    ) else (
        echo Please install MinGW and ensure g++ is in PATH.
        exit /b 1
    )
)

rem Path to libgit2 built with install_libgit2_mingw.bat
set "LIBGIT2_INC=%ROOT_DIR%\libs\libgit2\_install\include"
set "LIBGIT2_LIB=%ROOT_DIR%\libs\libgit2\_install\lib"

rem Build libgit2 if not already installed
if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call "%SCRIPT_DIR%install_libgit2_mingw.bat"
    if errorlevel 1 exit /b 1
)

if not exist dist mkdir dist
g++ -std=c++20 -static -I"%LIBGIT2_INC%" -Iinclude src\autogitpull.cpp src\git_utils.cpp src\tui.cpp src\logger.cpp src\resource_utils.cpp src\system_utils.cpp src\time_utils.cpp src\config_utils.cpp src\debug_utils.cpp src\parse_utils.cpp "%LIBGIT2_LIB%\libgit2.a" -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi -lyaml-cpp -o dist\autogitpull.exe

endlocal

