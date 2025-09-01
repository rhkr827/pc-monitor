@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d "D:\Repository\pc-monitor\backend-rust"

REM Add MinGW back to PATH for dlltool
set PATH=%PATH%;C:\Users\rhkr8\scoop\apps\mingw\current\bin

echo Environment set with both MSVC and MinGW tools
echo.
where cl
echo.
where dlltool
echo.
cargo build --verbose
pause
