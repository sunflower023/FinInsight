@echo off
REM FinInsight — MinGW one-click build (no Visual Studio / MSVC required)
REM Uses the MinGW toolchain bundled with Qt 6.7.3 and offline-cached deps in third_party/.
setlocal
set PATH=D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;D:\Qt\Tools\mingw1120_64\bin;%PATH%

cmake --preset win-mingw
if errorlevel 1 goto :fail

cmake --build --preset win-mingw --parallel 8
if errorlevel 1 goto :fail

echo.
echo Build OK: build\win-mingw\src\FinInsight.exe
goto :eof

:fail
echo.
echo Build FAILED.
exit /b 1
