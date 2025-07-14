@echo off
setlocal

rem Determine the vcpkg root (either via environment or local folder)
if "%VCPKG_ROOT%"=="" (
    if exist vcpkg (
        set "VCPKG_ROOT=%CD%\vcpkg"
    )
)

set "LIBGIT2_INC=%VCPKG_ROOT%\installed\x64-windows-static\include"
set "LIBGIT2_LIB=%VCPKG_ROOT%\installed\x64-windows-static\lib"

if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call install_deps.bat
)

g++ -std=c++17 -static -I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp "%LIBGIT2_LIB%\libgit2.a" -o autogitpull.exe

endlocal

