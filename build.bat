@echo off
echo ============================================
echo  [1/3] Activating VS 2022 MSVC environment...
echo ============================================
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if %ERRORLEVEL% NEQ 0 ( exit /b 1 )
set HTTPS_PROXY=http://127.0.0.1:7897

echo ============================================
echo  [2/3] CMake Configure...
echo ============================================
cd /d %~dp0
cmake --preset win-dev
if %ERRORLEVEL% NEQ 0 ( exit /b 1 )

echo ============================================
echo  [3/3] Compiling...
echo ============================================
cmake --build --preset win-dev --parallel 8
if %ERRORLEVEL% NEQ 0 ( exit /b 1 )

echo ============================================
echo  BUILD SUCCESS!  Run: build\win-dev\FinInsight.exe
echo ============================================
