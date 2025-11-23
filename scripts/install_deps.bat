@echo off
rem ------------------------------------------------------------------
rem DEPRECATED: helper to install third-party dependencies
rem ------------------------------------------------------------------
rem This project now uses CMake's FetchContent to obtain and build
rem required third-party libraries. In almost all cases you should
rem configure and build with CMake directly (see the README or
rem scripts/build.py wrapper) instead of running this script.
rem
rem If you are maintaining an older development setup or want to
rem bootstrap a native MinGW/VCPKG environment, you can still inspect
rem the legacy installers (install_libgit2_mingw.bat, etc.) but they
rem are no longer the recommended path.
echo [autogitpull] install_deps.bat is deprecated.
echo Recommended: use CMake + FetchContent. Example commands:
echo   cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
echo   cmake --build build --config Release --parallel 8
exit /b 0
