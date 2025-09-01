@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d "D:\Repository\pc-monitor\backend-rust"
echo PATH=%PATH%
echo.
echo LIB=%LIB%
echo.
echo LIBPATH=%LIBPATH%
echo.
echo INCLUDE=%INCLUDE%
echo.
where cl
echo.
where link
echo.
cargo build --verbose
pause
