@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d "D:\Repository\pc-monitor\backend-rust"

REM Remove MinGW paths that might conflict with MSVC
set PATH=%PATH:C:\MinGW\bin;=%
set PATH=%PATH:C:\Users\rhkr8\scoop\apps\mingw\current\bin;=%
set PATH=%PATH:C:\Users\rhkr8\scoop\apps\gcc\current\bin;=%

echo Cleaned PATH set
echo.
where cl
echo.
where link
echo.
cargo clean
cargo build --verbose
pause
