@echo off
setlocal EnableDelayedExpansion

rem *************************************************************
rem  AutoGitPull - compressed Windows build helper
rem  Builds using compile.bat with size optimizations and
rem  compresses the result using UPX.
rem *************************************************************

set "SCRIPT_DIR=%~dp0"
set "CXXFLAGS=-std=c++20 -Os -flto -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -s -DYAML_CPP_STATIC_DEFINE -Wl,--gc-sections -pipe -static -static-libgcc -static-libstdc++"

pushd "%SCRIPT_DIR%" || exit /b 1
call "%SCRIPT_DIR%compile.bat" %* || exit /b 1
popd

set "EXE=%SCRIPT_DIR%..\dist\autogitpull.exe"
if exist "%EXE%" (
    upx --best --lzma "%EXE%"
    echo Compressed build: %EXE%
) else (
    echo Build failed or executable missing.
    exit /b 1
)

endlocal
