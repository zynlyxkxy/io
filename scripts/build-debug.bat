@echo off
rem ============================================================
rem  Build Debug, then install the Debug variant of the "libraries
rem  provided to others" -> output\interface\rpc\lib\debug\
rem
rem  Usage:
rem    build-debug.bat                  build everything (default)
rem    build-debug.bat -t <target>      build only that target (+ its deps)
rem
rem  Why -t exists: VSCode runs a build before every launch (preLaunchTask).
rem  Building ALL targets would relink the exe of the OTHER configuration that
rem  is still running, and that fails with
rem      LINK : fatal error LNK1168: cannot open <exe> for writing
rem  after which VSCode aborts the launch. So each launch configuration builds
rem  only the target it is about to start (see .vscode\tasks.json).
rem
rem  The deploy shape (exe + runtime dll + qt.conf) is Release-only: its
rem  install rules carry CONFIGURATIONS Release, so this Debug install
rem  cannot pollute output\ with a debug exe.
rem
rem  build\Debug\ is self-contained, so both IDEs can start the exe directly.
rem  ASCII-only on purpose: see the note in build-release.bat.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TARGET="

call :main %*
set "RC=%ERRORLEVEL%"

if defined TARGET (
    set "SUMMARY_NOTE=Debug build (single target: %TARGET%)"
    call "%HERE%private_scripts\_lib.bat" summary %RC% "%ROOT%\build\Debug\%TARGET%.exe"
    exit /b %RC%
)

set "SUMMARY_NOTE=Debug build"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\build\Debug\CTL_MDL_IO.exe" ^
    "%ROOT%\build\Debug\GUI_MDL_IO.exe" ^
    "%ROOT%\build\Debug\iod.dll" ^
    "%ROOT%\output\interface\rpc\lib\debug\iod.dll" ^
    "%ROOT%\output\interface\rpc\lib\debug\iod.lib" ^
    "--" ^
    "%ROOT%\output\exe\CTL_MDL_IO.exe"
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
call "%HERE%private_scripts\_lib.bat" check_qtdir

if /i "%~1"=="-t"       set "TARGET=%~2"
if /i "%~1"=="--target" set "TARGET=%~2"

cmake --preset win64         || exit /b 1
if defined TARGET (
    cmake --build --preset debug --target %TARGET% || exit /b 1
    exit /b 0
)
cmake --build --preset debug || exit /b 1
exit /b 0
