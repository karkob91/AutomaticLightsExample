@echo off
setlocal

REM Change to the directory of the script
cd /d "%~dp0"

REM Read dznPath from config.json using PowerShell
for /f "delims=" %%p in ('powershell -NoProfile -Command "Get-Content config.json | ConvertFrom-Json | Select-Object -ExpandProperty dznPath"') do set DZN_PATH=%%p

REM Check if DZN_PATH is valid
if "%DZN_PATH%"=="" (
    echo Error: Unable to read Dezyne tool path from config.json.
    exit /b 1
)

REM Verify that dzn.cmd exists
if not exist "%DZN_PATH%\dzn.cmd" (
    echo Error: dzn.cmd not found in "%DZN_PATH%".
    exit /b 1
)

REM Print start message
echo.
echo === Verifying Models ===

REM Process all .dzn files in the models directory
for /r models %%f in (*.dzn) do (
    echo Verifying: %%~nxf
    call "%DZN_PATH%\dzn.cmd" verify -I models "%%f" || (
        echo Error verifying %%~nxf
        exit /b 1
    )
)

REM Print completion message
echo.
echo === Verification Completed ===

exit /b 0
