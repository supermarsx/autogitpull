@echo off
where libgit2.lib >nul 2>nul
if NOT %ERRORLEVEL%==0 (
    call install_deps.bat
)

cl /std:c++17 /EHsc autogitpull.cpp git_utils.cpp tui.cpp libgit2.lib

