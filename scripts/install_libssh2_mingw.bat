@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake is required! Please install cmake and ensure it is in PATH.
    exit /b 1
)

if not exist libs mkdir libs
if not exist libs\libssh2 (
    echo Cloning libssh2...
    git clone --depth 1 https://github.com/libssh2/libssh2 libs\libssh2 || exit /b 1
)

cd libs\libssh2
if exist build rmdir /s /q build
mkdir build
cd build

cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DCRYPTO_BACKEND=WinCNG -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=..\libssh2_install ..
if errorlevel 1 (
    echo libssh2 CMake configuration failed!
    exit /b 1
)

mingw32-make
if errorlevel 1 (
    echo libssh2 build failed!
    exit /b 1
)

mingw32-make install
if errorlevel 1 (
    echo libssh2 install failed!
    exit /b 1
)

cd ..\..

echo libssh2 built and installed to libs\libssh2\libssh2_install
endlocal
