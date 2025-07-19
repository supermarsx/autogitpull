@echo off
setlocal EnableDelayedExpansion

rem *************************************************************
rem  AutoGitPull - compressed Windows build helper
rem  Invokes compile.bat then compresses the result using UPX.
rem *************************************************************

pushd "%~dp0" || exit /b 1
call "%%~dp0compile.bat" %* || exit /b 1
popd

set "EXE=%~dp0..\dist\autogitpull.exe"
if exist "%EXE%" (
    upx --best --lzma "%EXE%"
    echo Compressed build: %EXE%
) else (
    echo Build failed or executable missing.
    exit /b 1
)

endlocal
