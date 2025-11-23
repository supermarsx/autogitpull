@echo off
echo ----------------------------------------------------------------
echo NOTE: install_nlohmann_json.bat is a legacy helper script.
echo The CMake build now fetches nlohmann/json automatically.
echo This script only exists as a fallback to bootstrap a very
echo old/local dev environment.
echo ----------------------------------------------------------------
setlocal

REM Fetch nlohmann/json single header (legacy fallback)

if not exist libs mkdir libs
if not exist libs\nlohmann-json (
    echo Cloning nlohmann/json...
    git clone --depth 1 https://github.com/nlohmann/json libs\nlohmann-json
)

echo nlohmann-json headers installed under libs\nlohmann-json
endlocal
