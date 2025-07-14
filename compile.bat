@echo off
setlocal

rem Determine the vcpkg root (either via environment or local folder)
if "%VCPKG_ROOT%"=="" (
    if exist vcpkg (
        set "VCPKG_ROOT=%CD%\vcpkg"
    )
)

set "LIBGIT2_INC=%VCPKG_ROOT%\installed\x64-windows\include"
set "LIBGIT2_LIB=%VCPKG_ROOT%\installed\x64-windows\lib"

if not exist "%LIBGIT2_LIB%\libgit2.lib" (
    call install_deps.bat
)

g++ -std=c++17 -I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp -L"%LIBGIT2_LIB%" -lgit2 -o git_auto_pull_all.exe

endlocal

