@echo off
REM Lightweight runner helper for a built autogitpull binary.
if "%~1"=="" (
	echo Usage: %~nx0 ^<path-to-repo-to-monitor^>
	echo Example: %~nx0 C:\Users\you\Projects\my-repo
	exit /b 1
)

set "EXE=dist\autogitpull.exe"
if not exist "%EXE%" (
	echo Binary %EXE% not found. Build first with scripts\build.py or CMake.
	exit /b 1
)

echo Running %EXE% against %~1
"%EXE%" "%~1"

