@echo off
setlocal

rem ──────────────────────────────────────────────────────────────
rem 1. Ensure clang/clang++ is available
where clang++ >nul 2>nul
if errorlevel 1 (
    echo clang++ not found. Attempting to install LLVM via Chocolatey...
    if defined ChocolateyInstall (
        choco install -y llvm
    ) else (
        echo Please install LLVM which provides clang and ensure it's in PATH."
        exit /b 1
    )
)

rem ──────────────────────────────────────────────────────────────
rem 2. libgit2 paths
set "LIBGIT2_INC=libgit2_install\include"
set "LIBGIT2_LIB=libgit2_install\lib"

if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call install_libgit2_mingw.bat
    if errorlevel 1 exit /b 1
)

rem ──────────────────────────────────────────────────────────────
rem 3. Build with clang++ and AddressSanitizer

rem Common flags
set "CXX=clang++"
set "CXXFLAGS=-std=c++20 -O0 -g -fsanitize=address"
set "LDFLAGS=-fsanitize=address"

%CXX% %CXXFLAGS% ^
    -I"%LIBGIT2_INC%" ^
    autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils.cpp system_utils.cpp time_utils.cpp debug_utils.cpp ^
    config_utils.cpp ^
    "%LIBGIT2_LIB%\libgit2.a" ^
    -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi -lyaml-cpp ^
    %LDFLAGS% ^
    -o autogitpull_debug.exe

if errorlevel 1 (
    echo Build failed.
    exit /b 1
) else (
    echo Build succeeded: autogitpull_debug.exe
)

endlocal
