@echo off
setlocal

where cl >nul 2>nul
if errorlevel 1 (
    echo MSVC compiler not found. Please run from a Developer Command Prompt.
    exit /b 1
)

if not defined VCPKG_ROOT (
    if exist vcpkg (
        set "VCPKG_ROOT=%CD%\vcpkg"
    )
)

if not exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    call install_deps.bat
    if errorlevel 1 exit /b 1
)

set "INCLUDE=%VCPKG_ROOT%\installed\x64-windows-static\include;%INCLUDE%"
set "LIB=%VCPKG_ROOT%\installed\x64-windows-static\lib;%LIB%"

cl /std:c++17 /EHsc /MT autogitpull.cpp git_utils.cpp tui.cpp ^
    /I"%VCPKG_ROOT%\installed\x64-windows-static\include" ^
    /link /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows-static\lib" git2.lib

endlocal

