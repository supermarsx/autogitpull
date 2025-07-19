@echo off
echo Installing libgit2 and related libraries...
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
if not exist libs mkdir ..\libs
REM Clone libgit2 into libs\libgit2 if missing
if not exist libs\libgit2 (
    echo Cloning libgit2...
    git clone --depth 1 https://github.com/libgit2/libgit2 ..\libs\libgit2
)

cd libs\libgit2

REM Clean any old builds
if exist build rmdir /s /q build

mkdir build
cd build

REM Configure for static build with MinGW
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=..\libgit2_install ..
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

echo libgit2 built and installed to libs\libgit2_install
cd ..\..

REM Build yaml-cpp if not already installed
if not exist ..\libs\yaml-cpp\yaml-cpp_install\lib\libyaml-cpp.a (
    if not exist ..\libs\yaml-cpp (
        echo Cloning yaml-cpp...
        git clone --depth 1 https://github.com/jbeder/yaml-cpp ..\libs\yaml-cpp
    )
    cd ..\libs\yaml-cpp
    if exist build rmdir /s /q build
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=..\yaml-cpp_install ..
    if errorlevel 1 (
        echo yaml-cpp CMake configuration failed!
        exit /b 1
    )
    mingw32-make
    if errorlevel 1 (
        echo yaml-cpp build failed!
        exit /b 1
    )
    mingw32-make install
    if errorlevel 1 (
        echo yaml-cpp install failed!
        exit /b 1
    )
    cd ..\..
)

REM Download nlohmann/json headers if not present
if not exist ..\libs\nlohmann-json\single_include\nlohmann\json.hpp (
    echo Downloading nlohmann/json...
    git clone --depth 1 https://github.com/nlohmann/json ..\libs\nlohmann-json
)

echo Dependencies installed under libs\yaml-cpp_install and libs\nlohmann-json

