@echo off
setlocal

REM Fetch nlohmann/json single header

if not exist libs mkdir libs
if not exist libs\nlohmann-json (
    echo Cloning nlohmann/json...
    git clone --depth 1 https://github.com/nlohmann/json libs\nlohmann-json
)

echo nlohmann-json headers installed under libs\nlohmann-json
endlocal
