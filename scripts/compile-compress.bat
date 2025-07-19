@echo off
setlocal
echo Compiling compressed executable...

REM Determine script directory
pushd "%~dp0\.."
  set "SCRIPT_DIR=%CD%\"
popd

REM Check for g++
where g++ >nul 2>nul
if errorlevel 1 (
    echo MinGW g++ not found.
    exit /b 1
)

set "LIBGIT2_INC=%SCRIPT_DIR%libs\libgit2\libgit2_install\include"
set "LIBGIT2_LIB=%SCRIPT_DIR%libs\libgit2\libgit2_install\lib"
set "YAMLCPP_INC=%SCRIPT_DIR%libs\yaml-cpp\yaml-cpp_install\include"
set "YAMLCPP_LIB=%SCRIPT_DIR%libs\yaml-cpp\yaml-cpp_install\lib"
set "JSON_INC=%SCRIPT_DIR%libs\nlohmann-json\single_include"

if not exist "%LIBGIT2_LIB%\libgit2.a" (
    call "%SCRIPT_DIR%scripts\install_libgit2_mingw.bat" || exit /b 1
)
if not exist "%YAMLCPP_LIB%\libyaml-cpp.a" (
    call "%SCRIPT_DIR%scripts\install_libgit2_mingw.bat" || exit /b 1
)
if not exist "%JSON_INC%\nlohmann\json.hpp" (
    call "%SCRIPT_DIR%scripts\install_libgit2_mingw.bat" || exit /b 1
)

if not exist "%SCRIPT_DIR%dist" mkdir "%SCRIPT_DIR%dist"

windres ^
    "%SCRIPT_DIR%src\version.rc" ^
    -I "%SCRIPT_DIR%include" ^
    -O coff -o "%SCRIPT_DIR%src\version.o"

REM Compile with size optimizations
"g++" -std=c++20 -Os -flto -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -s -DYAML_CPP_STATIC_DEFINE ^
    -Wl,--gc-sections ^
    -I "%LIBGIT2_INC%" ^
    -I "%YAMLCPP_INC%" ^
    -I "%JSON_INC%" ^
    -I "%SCRIPT_DIR%include" ^
    "%SCRIPT_DIR%src\autogitpull.cpp" ^
    "%SCRIPT_DIR%src\git_utils.cpp" ^
    "%SCRIPT_DIR%src\tui.cpp" ^
    "%SCRIPT_DIR%src\logger.cpp" ^
    "%SCRIPT_DIR%src\resource_utils.cpp" ^
    "%SCRIPT_DIR%src\system_utils.cpp" ^
    "%SCRIPT_DIR%src\time_utils.cpp" ^
    "%SCRIPT_DIR%src\config_utils.cpp" ^
    "%SCRIPT_DIR%src\debug_utils.cpp" ^
    "%SCRIPT_DIR%src\options.cpp" ^
    "%SCRIPT_DIR%src\parse_utils.cpp" ^
    "%SCRIPT_DIR%src\lock_utils.cpp" ^
    "%SCRIPT_DIR%src\linux_daemon.cpp" ^
    "%SCRIPT_DIR%src\windows_service.cpp" ^
    "%SCRIPT_DIR%src\version.o" ^
    "%LIBGIT2_LIB%\libgit2.a" ^
    "%YAMLCPP_LIB%\libyaml-cpp.a" ^
    -lssh2 -lz -lws2_32 -lwinhttp -lole32 -lrpcrt4 -lcrypt32 -lpsapi -ladvapi32 ^
    -o "%SCRIPT_DIR%dist\autogitpull.exe"

strip "%SCRIPT_DIR%dist\autogitpull.exe"

if exist "%SCRIPT_DIR%dist\autogitpull.exe" (
    upx --best --lzma "%SCRIPT_DIR%dist\autogitpull.exe"
    echo Build succeeded: %SCRIPT_DIR%dist\autogitpull.exe
)

endlocal

