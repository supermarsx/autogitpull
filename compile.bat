@echo off
setlocal

rem Path to libgit2 built with install_libgit2_mingw.bat
set "LIBGIT2_INC=libgit2\_install\include"
set "LIBGIT2_LIB=libgit2\_install\lib"

rem Build libgit2 if not already installed
if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call install_libgit2_mingw.bat
    if errorlevel 1 exit /b 1
)

g++ -std=c++17 -static -I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp "%LIBGIT2_LIB%\libgit2.a" -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -o autogitpull.exe

endlocal

