@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "BUNDLED_PYTHON=%SCRIPT_DIR%Scripts\python\python.exe"
set "GENERATOR=%SCRIPT_DIR%Scripts\GenerateProjectFiles.py"

if not exist "%GENERATOR%" (
    echo GenerateProjectFiles.py not found: %GENERATOR%
    goto :Fail
)

if exist "%BUNDLED_PYTHON%" (
    "%BUNDLED_PYTHON%" "%GENERATOR%" %*
    goto :AfterRun
)

where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%GENERATOR%" %*
    goto :AfterRun
)

where python >nul 2>nul
if not errorlevel 1 (
    python "%GENERATOR%" %*
    goto :AfterRun
)

echo Python not found. Expected bundled Python at:
echo   %BUNDLED_PYTHON%
goto :Fail

:AfterRun
if errorlevel 1 goto :Fail
echo.
echo Project files generated successfully.
pause
exit /b 0

:Fail
echo.
echo GenerateProjectFiles failed.
pause
exit /b 1
