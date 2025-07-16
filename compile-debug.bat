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

set "LIBGIT2_INC=libgit2\_install\include"
set "LIBGIT2_LIB=libgit2\_install\lib"

if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call install_libgit2_mingw.bat
    if errorlevel 1 exit /b 1
)

rem Detect availability of AddressSanitizer
for /f "delims=" %%d in ('g++ -print-file-name=libasan.a') do set "ASAN_LIB=%%d"
if exist "%ASAN_LIB%" (
    set "ASAN_FLAGS=-fsanitize=address"
    echo Using AddressSanitizer library: %ASAN_LIB%
) else (
    echo AddressSanitizer not found. Building without it.
    set "ASAN_FLAGS="
)

g++ -std=c++20 -O0 -g %ASAN_FLAGS% -I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils.cpp system_utils.cpp time_utils.cpp "%LIBGIT2_LIB%\libgit2.a" -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi %ASAN_FLAGS% -o autogitpull_debug.exe

endlocal
