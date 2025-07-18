@echo off
setlocal
echo Generating platform icons...

REM Generate platform icons from graphics\icon.png
pushd "%~dp0\.."
set "SRC=%CD%\graphics\icon.png"
set "DST=%CD%\graphics"

REM Check for ImageMagick's convert
where convert >nul 2>nul
if errorlevel 1 (
    echo ImageMagick not found. Cloning and building from source...
    set "TMPDIR=%TEMP%\im-build-%RANDOM%"
    if exist "%TMPDIR%" rmdir /s /q "%TMPDIR%"
    mkdir "%TMPDIR%"
    pushd "%TMPDIR%"
    git clone --depth 1 https://github.com/ImageMagick/ImageMagick ImageMagick
    pushd ImageMagick
    if exist configure (
        bash configure --prefix="%TMPDIR%\im-install"
        if errorlevel 1 (
            echo configure failed
            popd
            popd
            exit /b 1
        )
        mingw32-make -j%NUMBER_OF_PROCESSORS%
        mingw32-make install
        set "PATH=%TMPDIR%\im-install\bin;%PATH%"
    ) else (
        echo configure script not found. Please install ImageMagick manually.
        popd
        popd
        exit /b 1
    )
    popd
    popd
)

cd "%DST%"

REM Windows ICO
if not exist icon.ico (
    convert "%SRC%" -resize 16x16 icon_16.png
    convert "%SRC%" -resize 32x32 icon_32.png
    convert "%SRC%" -resize 48x48 icon_48.png
    convert "%SRC%" -resize 256x256 icon_256.png
    convert icon_16.png icon_32.png icon_48.png icon_256.png icon.ico
)

REM macOS ICNS
if not exist icon.icns (
    mkdir icon.iconset 2>nul
    convert "%SRC%" -resize 16x16 icon.iconset\icon_16x16.png
    convert "%SRC%" -resize 32x32 icon.iconset\icon_16x16@2x.png
    convert "%SRC%" -resize 32x32 icon.iconset\icon_32x32.png
    convert "%SRC%" -resize 64x64 icon.iconset\icon_32x32@2x.png
    convert "%SRC%" -resize 128x128 icon.iconset\icon_128x128.png
    convert "%SRC%" -resize 256x256 icon.iconset\icon_128x128@2x.png
    convert "%SRC%" -resize 256x256 icon.iconset\icon_256x256.png
    convert "%SRC%" -resize 512x512 icon.iconset\icon_256x256@2x.png
    convert "%SRC%" -resize 512x512 icon.iconset\icon_512x512.png
    convert "%SRC%" -resize 1024x1024 icon.iconset\icon_512x512@2x.png
    if exist "%ProgramFiles%\icnsutil\bin\png2icns.exe" (
        "%ProgramFiles%\icnsutil\bin\png2icns.exe" icon.icns icon.iconset\icon_*.png
    ) else (
        echo png2icns not found; skipping ICNS generation
    )
)

REM Linux PNGs
for %%S in (16 32 48 128 256) do (
    if not exist icon_%%S.png convert "%SRC%" -resize %%Sx%%S icon_%%S.png
)

popd

echo Icons generated in %DST%
endlocal

