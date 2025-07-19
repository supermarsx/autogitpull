@echo off
setlocal enableextensions enabledelayedexpansion
echo =========================================
echo Generating platform icons...
echo =========================================

REM -- Move to project root (assumes script is in scripts\)
pushd "%~dp0.." || exit /b 1
set "SRC=%CD%\graphics\icon.png"
set "DST=%CD%\graphics"

echo [Step] Installing ImageMagick via winget if necessary
where magick >nul 2>&1 || (
    echo [Info] Installing ImageMagick using winget...
    winget install --id ImageMagick.ImageMagick -e --silent
    if errorlevel 1 (
        echo [Error] winget installation failed; please install ImageMagick manually.
        exit /b 1
    )
)
set "IM_CMD=magick"

echo [Step] Changing directory to %DST%
cd /d "%DST%" || exit /b 1

REM -- Windows ICO generation
echo [Step] Generating Windows .ico files...
if not exist icon.ico (
    "%IM_CMD%" "%SRC%" -resize 16x16   icon_16.png
    "%IM_CMD%" "%SRC%" -resize 32x32   icon_32.png
    "%IM_CMD%" "%SRC%" -resize 48x48   icon_48.png
    "%IM_CMD%" "%SRC%" -resize 256x256 icon_256.png
    "%IM_CMD%" icon_16.png icon_32.png icon_48.png icon_256.png icon.ico
    echo [Done] icon.ico created in %DST%
) else (
    echo [Skip] icon.ico already exists
)

REM -- macOS .icns generation using Python icnsutil
echo [Step] Generating macOS .icns file...
if not exist icon.icns (
    if not exist icon.iconset mkdir icon.iconset
    "%IM_CMD%" "%SRC%" -resize 16x16   icon.iconset\icon_16x16.png
    "%IM_CMD%" "%SRC%" -resize 32x32   icon.iconset\icon_16x16@2x.png
    "%IM_CMD%" "%SRC%" -resize 32x32   icon.iconset\icon_32x32.png
    "%IM_CMD%" "%SRC%" -resize 64x64   icon.iconset\icon_32x32@2x.png
    "%IM_CMD%" "%SRC%" -resize 128x128 icon.iconset\icon_128x128.png
    "%IM_CMD%" "%SRC%" -resize 256x256 icon.iconset\icon_128x128@2x.png
    "%IM_CMD%" "%SRC%" -resize 256x256 icon.iconset\icon_256x256.png
    "%IM_CMD%" "%SRC%" -resize 512x512 icon.iconset\icon_256x256@2x.png
    "%IM_CMD%" "%SRC%" -resize 512x512 icon.iconset\icon_512x512.png
    "%IM_CMD%" "%SRC%" -resize 1024x1024 icon.iconset\icon_512x512@2x.png
    echo [Info] Ensuring icnsutil is installed
    python -m pip install --upgrade icnsutil
    if errorlevel 1 (
        echo [Error] Failed to install 'icnsutil'; please install it manually.
    )
    echo [Info] Composing .icns file using icnsutil CLI
    python -m icnsutil compose icon.icns icon.iconset\icon_16x16.png icon.iconset\icon_16x16@2x.png icon.iconset\icon_32x32.png icon.iconset\icon_32x32@2x.png icon.iconset\icon_128x128.png icon.iconset\icon_128x128@2x.png icon.iconset\icon_256x256.png icon.iconset\icon_256x256@2x.png icon.iconset\icon_512x512.png icon.iconset\icon_512x512@2x.png
    if errorlevel 1 (
        echo [Error] icnsutil CLI failed; ensure 'icnsutil' is installed and in PATH.
    ) else (
        echo [Done] icon.icns created in %DST%
    )
) else (
    echo [Skip] icon.icns already exists
)

REM -- Linux PNG variants
echo [Step] Generating Linux PNG variants...
for %%S in (16 32 48 128 256) do (
    if not exist icon_%%S.png (
        "%IM_CMD%" "%SRC%" -resize %%Sx%%S icon_%%S.png && echo [Done] icon_%%S.png
    ) else (
        echo [Skip] icon_%%S.png
    )
)

echo =========================================
echo All icons are generated in %DST%
echo =========================================
popd
endlocal
