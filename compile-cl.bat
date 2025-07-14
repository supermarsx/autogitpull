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

cl /std:c++17 /EHsc /I"%LIBGIT2_INC%" autogitpull.cpp git_utils.cpp tui.cpp /link /LIBPATH:"%LIBGIT2_LIB%" libgit2.lib

endlocal

