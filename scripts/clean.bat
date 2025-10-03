@echo off
echo Cleaning build artifacts...
if exist autogitpull.exe del /f /q autogitpull.exe
if exist autogitpull del /f /q autogitpull
for %%f in (*.o *.obj) do (
    if exist "%%f" del /f /q "%%f"
)
if exist build (
    echo Removing build directory...
    rmdir /s /q build
)
if exist dist (
    echo Removing dist directory...
    rmdir /s /q dist
)
echo Clean complete.
