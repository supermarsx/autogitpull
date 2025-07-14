@echo off
where libgit2.lib >nul 2>nul
if NOT %ERRORLEVEL%==0 (
    call install_deps.bat
)

g++ -std=c++17 -lgit2 -o git_auto_pull_all.exe autogitpull.cpp git_utils.cpp tui.cpp

