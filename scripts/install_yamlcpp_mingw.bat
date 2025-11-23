@echo off
echo ----------------------------------------------------------------
echo NOTE: install_yamlcpp_mingw.bat is a legacy helper script.
echo Prefer the CMake fetch/build flow (FetchContent) instead of
echo running this script manually; this file is retained as a fallback.
echo ----------------------------------------------------------------
setlocal

REM Install yaml-cpp using MinGW and CMake

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake is required! Please install cmake and ensure it is in PATH.
    exit /b 1
)

if not exist libs mkdir libs
if not exist libs\yaml-cpp (
    echo Cloning yaml-cpp...
    git clone --depth 1 https://github.com/jbeder/yaml-cpp libs\yaml-cpp
)
cd libs\yaml-cpp
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

echo yaml-cpp built and installed to libs\yaml-cpp_install
endlocal
