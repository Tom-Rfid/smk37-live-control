@echo off
rem Simulator tests of the LIVE / CONTROL MAP feature on Windows.
rem Uses the MinGW gcc installed with CodeBlocks (or set MINGW_BIN to another bin folder).
rem   run.bat          build (debug, -g -O0) and run the tests
rem   run.bat build    build only (used by the VS Code debugger)
setlocal enabledelayedexpansion
cd /d "%~dp0..\.."
if not defined MINGW_BIN (
  if exist "C:\Program Files\CodeBlocks\MinGW\bin\gcc.exe" set "MINGW_BIN=C:\Program Files\CodeBlocks\MinGW\bin"
  if exist "C:\Program Files (x86)\CodeBlocks\MinGW\bin\gcc.exe" set "MINGW_BIN=C:\Program Files (x86)\CodeBlocks\MinGW\bin"
)
if defined MINGW_BIN set "PATH=%MINGW_BIN%;%PATH%"
gcc --version >nul 2>nul
if errorlevel 1 (
  echo gcc introuvable : installez CodeBlocks avec MinGW ou definissez MINGW_BIN.
  exit /b 1
)
if not exist build\host-tests mkdir build\host-tests
set "SRC="
rem app\*.c only: app\board\ is the real keyboard hardware, replaced by web\engine\Simulator.c
for %%f in (app\*.c) do set "SRC=!SRC! "%%f""
for /r web %%f in (*.c) do set "SRC=!SRC! "%%f""
for /r third_party\synth %%f in (*.c) do set "SRC=!SRC! "%%f""
gcc -std=c11 -g -O0 -fwrapv -fno-fast-math -ffp-contract=off -DFM1_HOST_TEST -DFM1_OPEN_WEB -w ^
  -Iweb/engine -Iweb/synth -Ithird_party/synth -Isdk/include -Iapp/synth -Iapp ^
  !SRC! tools/host-tests/map_test.c -o build/host-tests/map_test.exe -lm
if errorlevel 1 exit /b 1
if /i "%~1"=="build" exit /b 0
build\host-tests\map_test.exe
