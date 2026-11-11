@echo off
rem ============================================================
rem  Build Release; output\exe\ + output\interface\ are produced by the
rem  "deploy" target that is part of the default build
rem  (see cmake\istar_install.cmake : both dirs are replaced wholesale).
rem  Touches no git, no other project - usable standalone.
rem
rem  Usage:
rem    build-release.bat                build everything + install (default)
rem    build-release.bat -t <target>    build only that target (+ its deps),
rem                                     no install
rem
rem  -t is for VSCode launch configurations: building ALL targets would relink
rem  the exe of the other configuration that is still running -> LNK1168 ->
rem  the launch is aborted. See the same note in build-debug.bat.
rem
rem  NOTE: .bat files must stay ASCII-only. cmd parses them with the console
rem  code page (GBK/936 here), so UTF-8 Chinese comments can swallow the line
rem  ending and corrupt the next line - depending on byte alignment.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TARGET="

call :main %*
set "RC=%ERRORLEVEL%"

if defined TARGET (
    set "SUMMARY_NOTE=Release build (single target: %TARGET%, no install)"
    call "%HERE%private_scripts\_lib.bat" summary %RC% "%ROOT%\build\Release\%TARGET%.exe"
    exit /b %RC%
)

set "SUMMARY_NOTE=Release build"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\build\Release\CTL_MDL_IO.exe" ^
    "%ROOT%\build\Release\GUI_MDL_IO.exe" ^
    "%ROOT%\output\exe\CTL_MDL_IO.exe" ^
    "%ROOT%\output\exe\GUI_MDL_IO.exe" ^
    "%ROOT%\output\exe\io.dll" ^
    "%ROOT%\output\exe\qt.conf" ^
    "--" ^
    "%ROOT%\output\interface\rpc\lib\debug\iod.dll"
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
call "%HERE%private_scripts\_lib.bat" check_qtdir

if /i "%~1"=="-t"       set "TARGET=%~2"
if /i "%~1"=="--target" set "TARGET=%~2"

cmake --preset win64           || exit /b 1
if defined TARGET (
    cmake --build --preset release --target %TARGET% || exit /b 1
    exit /b 0
)
cmake --build --preset release || exit /b 1
exit /b 0
