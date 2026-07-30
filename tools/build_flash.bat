@echo off
setlocal

set "PROJECT_DIR=%~dp0.."
set "OPENOCD=D:\ti\xpack-openocd-0.12.0-7\bin\openocd.exe"
set "OPENOCD_SCRIPTS=D:\ti\xpack-openocd-0.12.0-7\openocd\scripts"
set "ELF=%PROJECT_DIR:\=/%/Debug/empty_mspm0g3507.out"

rem ??: program + verify + reset + exit
rem /halt:   program + verify + exit (???)
rem /quick:  program + reset + exit    (????, ???) (???, ?? halt ??? CCS ??)
set "PROGRAM_COMMAND=program %ELF% verify reset exit"
if /I "%~1"=="/halt"   set "PROGRAM_COMMAND=program %ELF% verify exit"
if /I "%~1"=="/quick"  set "PROGRAM_COMMAND=program %ELF% reset exit"

echo [1/2] Building project...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_project.ps1"
if errorlevel 1 (
    echo Build failed. Flash was NOT changed.
    exit /b 1
)

echo.
echo [2/2] Programming with nanoDAP...
"%OPENOCD%" ^
    -s "%OPENOCD_SCRIPTS%" ^
    -f interface/cmsis-dap.cfg ^
    -c "cmsis-dap backend hid" ^
    -c "cmsis-dap quirk enable" ^
    -c "transport select swd" ^
    -c "adapter speed 2000" ^
    -f target/ti_mspm0.cfg ^
    -c "%PROGRAM_COMMAND%"

if errorlevel 1 (
    echo.
    echo ============================================
    echo Programming FAILED.
    echo Check:
    echo   1. Target power (3.3V / USB)
    echo   2. GND connected
    echo   3. SWDIO ^& SWCLK connected
    echo   4. nRST connected (optional, try without)
    echo ============================================
    exit /b 1
)

echo.
echo ============================================
echo Programming completed successfully.
echo ============================================
exit /b 0
