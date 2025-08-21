@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."

echo Compiling debug build with MSVC...
where cl >nul 2>nul || (
    echo MSVC cl compiler not found in PATH.
    exit /b 1
)

if not exist "%VCPKG_ROOT%\installed\x64-windows-static\lib\git2.lib" (
    call "%SCRIPT_DIR%install_deps.bat" || exit /b 1
)

set "INC=%VCPKG_ROOT%\installed\x64-windows-static\include"
set "LIB=%VCPKG_ROOT%\installed\x64-windows-static\lib"
if not exist dist mkdir dist

cl /nologo /std:c++20 /EHsc /Zi /D YAML_CPP_STATIC_DEFINE ^
 /I "%INC%" /I include ^
 src\autogitpull.cpp src\scanner.cpp src\ui_loop.cpp src\git_utils.cpp src\tui.cpp src\logger.cpp ^
 src\resource_utils.cpp src\system_utils.cpp src\time_utils.cpp src\config_utils.cpp src\debug_utils.cpp ^
 src\ignore_utils.cpp ^
 src\options.cpp src\parse_utils.cpp src\mutant_mode.cpp src\lock_utils_windows.cpp src\process_monitor.cpp src\help_text.cpp ^
 src\cli_commands.cpp src\linux_daemon.cpp src\windows_service.cpp ^
 src\linux_commands.cpp src\windows_commands.cpp ^
 "%LIB%\git2.lib" advapi32.lib Ws2_32.lib Shell32.lib Ole32.lib Rpcrt4.lib Crypt32.lib winhttp.lib Psapi.lib yaml-cpp.lib /fsanitize:address ^
 /Fedist\autogitpull_debug.exe

endlocal
