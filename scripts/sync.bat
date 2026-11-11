@echo off
rem ============================================================
rem  Sync this project's protocol definition headers downstream:
rem    io -> mctrl\MainProject\RPCModule\include\io   (A1)
rem
rem  The engine lives at the top level (1927project\scripts\private_scripts\_sync_ops.bat) so
rem  all cross-project writes follow one table; this is only a forward.
rem  Identical content is skipped WITHOUT touching the target mtime, so
rem  downstream regeneration/compilation is not needlessly retriggered.
rem
rem  Extra arguments are passed through, e.g.:
rem      sync.bat --dry-run      report only, write nothing
rem
rem  ASCII-only on purpose: see the note in build-release.bat.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TOP=%ROOT%\..\..\engineering\scripts"

if not exist "%TOP%\private_scripts\_sync_ops.bat" (
    echo [SKIP] top-level sync engine not found: %TOP%\private_scripts\_sync_ops.bat
    echo        Nothing was synced. Build the top level, or run private_scripts\_sync_ops.bat there.
    exit /b 0
)

call "%TOP%\private_scripts\_sync_ops.bat" proj io %*
set "RC=%ERRORLEVEL%"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\..\..\mctrl\MainProject\RPCModule\include\io\io_controller.h"
exit /b %RC%
