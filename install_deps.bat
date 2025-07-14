@echo off
where libgit2.lib >nul 2>nul
if %ERRORLEVEL%==0 (
    echo libgit2 already installed.
    goto :eof
)

if exist vcpkg (
    echo Installing libgit2 via vcpkg...
    vcpkg\vcpkg install libgit2
    goto :eof
)

echo libgit2 not found. Downloading vcpkg to install...
where git >nul 2>nul
if errorlevel 1 (
    echo Git is required to download vcpkg. Please install Git and retry.
    exit /b 1
)

git clone https://github.com/microsoft/vcpkg
cd vcpkg
call bootstrap-vcpkg.bat
cd ..
vcpkg\vcpkg install libgit2

