@echo off
setlocal
set "SCRIPT_DIR=%~dp0"

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
rem 2. dependency paths
set "LIBGIT2_INC=libs\libgit2\_install\include"
set "LIBGIT2_LIB=libs\libgit2\_install\lib"
set "YAMLCPP_INC=libs\yaml-cpp\yaml-cpp_install\include"
set "YAMLCPP_LIB=libs\yaml-cpp\yaml-cpp_install\lib"
set "JSON_INC=libs\nlohmann-json\single_include"

if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call "%SCRIPT_DIR%install_libgit2_mingw.bat"
    if errorlevel 1 exit /b 1
)
if not exist "%YAMLCPP_LIB%\libyaml-cpp.a" (
    call "%SCRIPT_DIR%install_libgit2_mingw.bat"
    if errorlevel 1 exit /b 1
)
if not exist "%JSON_INC%\nlohmann\json.hpp" (
    call "%SCRIPT_DIR%install_libgit2_mingw.bat"
    if errorlevel 1 exit /b 1
)

rem ──────────────────────────────────────────────────────────────
rem 3. Build with clang++ and AddressSanitizer

rem Common flags
set "CXX=clang++"
set "CXXFLAGS=-std=c++20 -O0 -g -fsanitize=address -DYAML_CPP_STATIC_DEFINE"
set "LDFLAGS=-fsanitize=address"

if not exist dist mkdir dist

%CXX% %CXXFLAGS% ^
    -I"%LIBGIT2_INC%" -I"%YAMLCPP_INC%" -I"%JSON_INC%" -Iinclude ^
    src\autogitpull.cpp src\git_utils.cpp src\tui.cpp src\logger.cpp src\resource_utils.cpp src\system_utils.cpp src\time_utils.cpp src\debug_utils.cpp src\parse_utils.cpp src\lock_utils.cpp src\linux_daemon.cpp ^
    src\config_utils.cpp src\options.cpp ^
    "%LIBGIT2_LIB%\libgit2.a" ^
    "%YAMLCPP_LIB%\libyaml-cpp.a" ^
    -lz -lssh2 -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi ^
    %LDFLAGS% ^
    -o dist\autogitpull_debug.exe

if errorlevel 1 (
    echo Build failed.
    exit /b 1
) else (
    echo Build succeeded: dist\autogitpull_debug.exe
)

endlocal
