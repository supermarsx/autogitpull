@echo off
setlocal

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
set "LIBGIT2_INC=libgit2\_install\include"
set "LIBGIT2_LIB=libgit2\_install\lib"

rem Build libgit2 if not already installed
if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call install_libgit2_mingw.bat
    if errorlevel 1 exit /b 1
)

g++ -std=c++20 -static -I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils.cpp system_utils.cpp time_utils.cpp "%LIBGIT2_LIB%\libgit2.a" -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi -o autogitpull.exe

endlocal

