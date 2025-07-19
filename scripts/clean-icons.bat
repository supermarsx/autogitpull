@echo off
setlocal enableextensions enabledelayedexpansion
echo =========================================
echo Cleaning up generated icons...
echo =========================================

REM -- Move to project root (assumes script is in scripts\)
pushd "%~dp0.." || exit /b 1
set "DST=%CD%\graphics"

echo [Step] Changing directory to %DST%
cd /d "%DST%" || exit /b 1

REM -- Remove Windows ICO and macOS ICNS
del /q icon.ico 2>nul && echo [Info] Deleted icon.ico
del /q icon.icns 2>nul && echo [Info] Deleted icon.icns

REM -- Remove PNG variants (common sizes)
for %%S in (16 32 48 128 256 512 1024) do (
    del /q icon_%%S.png 2>nul && echo [Info] Deleted icon_%%S.png
)

REM -- Remove the icon.iconset folder if it exists
if exist icon.iconset (
    echo [Info] Removing icon.iconset directory...
    rmdir /s /q icon.iconset
)

echo =========================================
echo Cleanup complete!
echo =========================================
popd
endlocal
