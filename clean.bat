@echo off
if exist autogitpull.exe del /f /q autogitpull.exe
if exist autogitpull del /f /q autogitpull
for %%f in (*.o *.obj) do (
    if exist "%%f" del /f /q "%%f"
)
if exist build (
    rmdir /s /q build
)
