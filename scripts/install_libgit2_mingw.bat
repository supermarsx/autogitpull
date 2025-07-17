@echo off
REM Check for MinGW g++
where g++ >nul 2>nul
if errorlevel 1 (
    echo MinGW g++ not found in PATH. Attempting to install...
    if defined ChocolateyInstall (
        choco install -y mingw
    ) else (
        echo Please install MinGW and ensure g++ is in PATH.
        exit /b 1
    )
)

REM Check for CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake is required! Please install cmake and make sure it's on your PATH.
    exit /b 1
)

REM Download libgit2 if not present
if not exist libgit2 (
    echo Cloning libgit2...
    git clone --depth 1 https://github.com/libgit2/libgit2
)

cd libgit2

REM Clean any old builds
if exist build rmdir /s /q build

mkdir build
cd build

REM Configure for static build with MinGW
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=..\_install ..
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build and install
mingw32-make
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

mingw32-make install
if errorlevel 1 (
    echo Install failed!
    exit /b 1
)

echo libgit2 built and installed to libgit2\_install
cd ..\..
