@echo off
setlocal
set "SCRIPT_DIR=%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
    echo MSVC cl compiler not found in PATH.
    exit /b 1
)

if not exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    call "%SCRIPT_DIR%install_deps.bat"
    if errorlevel 1 exit /b 1
)

set "INC=%VCPKG_ROOT%\installed\x64-windows-static\include"
set "LIB=%VCPKG_ROOT%\installed\x64-windows-static\lib"

cl /nologo /std:c++20 /EHsc /Zi /I"%INC%" autogitpull.cpp git_utils.cpp tui.cpp logger.cpp resource_utils.cpp system_utils.cpp time_utils.cpp config_utils.cpp debug_utils.cpp ^
    "%LIB%\git2.lib" advapi32.lib Ws2_32.lib Shell32.lib Ole32.lib Rpcrt4.lib Crypt32.lib winhttp.lib Psapi.lib yaml-cpp.lib /fsanitize=address /Feautogitpull_debug.exe

endlocal
