@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."

set "CXX=g++"
where %CXX% >nul 2>nul || (
    echo Compiler %CXX% not found.
    exit /b 1
)

set "CXXFLAGS=-std=c++20 -O0 -g -fsanitize=address -DYAML_CPP_STATIC_DEFINE"
set "LDFLAGS=-fsanitize=address"
set "INC=%ROOT_DIR%\include"
set "LIBGIT2_INC=%ROOT_DIR%\libs\libgit2\libgit2_install\include"
set "LIBGIT2_LIB=%ROOT_DIR%\libs\libgit2\libgit2_install\lib"
set "YAMLCPP_INC=%ROOT_DIR%\libs\yaml-cpp\yaml-cpp_install\include"
set "YAMLCPP_LIB=%ROOT_DIR%\libs\yaml-cpp\yaml-cpp_install\lib"
set "JSON_INC=%ROOT_DIR%\libs\nlohmann-json\single_include"
set "LIBSSH2_LIB=%ROOT_DIR%\libs\libssh2\libssh2_install\lib"
if not exist "%LIBSSH2_LIB%\libssh2.a" call "%SCRIPT_DIR%install_libssh2_mingw.bat" || exit /b 1
if not exist "%LIBGIT2_LIB%\libgit2.a" call "%SCRIPT_DIR%install_libgit2_mingw.bat" || exit /b 1
if not exist "%YAMLCPP_LIB%\libyaml-cpp.a" call "%SCRIPT_DIR%install_yamlcpp_mingw.bat" || exit /b 1
if not exist "%JSON_INC%\nlohmann\json.hpp" call "%SCRIPT_DIR%install_nlohmann_json.bat" || exit /b 1

if not exist "%ROOT_DIR%\dist" mkdir "%ROOT_DIR%\dist"

set "SSH2_LIB="
if exist "%LIBSSH2_LIB%\libssh2.a" (
    set "SSH2_LIB=%LIBSSH2_LIB%\libssh2.a"
) else (
    echo libssh2 not found, skipping SSH support.
)

%CXX% %CXXFLAGS% -I"%INC%" -I"%LIBGIT2_INC%" -I"%YAMLCPP_INC%" -I"%JSON_INC%" ^
    "%ROOT_DIR%\src\autogitpull.cpp" "%ROOT_DIR%\src\scanner.cpp" "%ROOT_DIR%\src\ui_loop.cpp" ^
    "%ROOT_DIR%\src\file_watch.cpp" ^
    "%ROOT_DIR%\src\git_utils.cpp" "%ROOT_DIR%\src\tui.cpp" "%ROOT_DIR%\src\logger.cpp" ^
    "%ROOT_DIR%\src\resource_utils.cpp" "%ROOT_DIR%\src\system_utils.cpp" "%ROOT_DIR%\src\time_utils.cpp" ^
    "%ROOT_DIR%\src\config_utils.cpp" "%ROOT_DIR%\src\ignore_utils.cpp" "%ROOT_DIR%\src\debug_utils.cpp" "%ROOT_DIR%\src\options.cpp" ^
    "%ROOT_DIR%\src\parse_utils.cpp" "%ROOT_DIR%\src\history_utils.cpp" "%ROOT_DIR%\src\mutant_mode.cpp" "%ROOT_DIR%\src\lock_utils_windows.cpp" "%ROOT_DIR%\src\process_monitor.cpp" ^
    "%ROOT_DIR%\src\help_text.cpp" "%ROOT_DIR%\src\cli_commands.cpp" "%ROOT_DIR%\src\linux_daemon.cpp" "%ROOT_DIR%\src\windows_service.cpp" ^
    "%ROOT_DIR%\src\linux_commands.cpp" "%ROOT_DIR%\src\windows_commands.cpp" ^
    "%LIBGIT2_LIB%\libgit2.a" "%YAMLCPP_LIB%\libyaml-cpp.a" %SSH2_LIB% -lz -lws2_32 -lwinhttp ^
    -lole32 -lrpcrt4 -lcrypt32 -lpsapi %LDFLAGS% -o "%ROOT_DIR%\dist\autogitpull_debug.exe"

if errorlevel 1 (
    echo Build failed.
    exit /b 1
) else (
    echo Build succeeded: %ROOT_DIR%\dist\autogitpull_debug.exe
)

endlocal
