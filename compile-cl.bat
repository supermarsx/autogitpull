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

rem Newer versions may name the library git2.lib
if not exist "%LIBGIT2_LIB%\git2.lib" (
    call install_deps.bat
)

cl /std:c++17 /EHsc /MT /I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp /link /LIBPATH:"%LIBGIT2_LIB%" git2.lib

endlocal

