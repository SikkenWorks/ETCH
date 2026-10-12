@echo off
setlocal
REM Runs host-side tests under Windows Subsystem for Linux (Ubuntu).
REM Does not connect to, write to, or update the FM-1 hardware.
wsl.exe --cd "%~dp0.." bash host/run_host_tests.sh
if errorlevel 1 (
  echo.
  echo HOST TEST FAILED. Check that WSL/Ubuntu, gcc and python3 are installed.
  pause
  exit /b 1
)
echo.
echo Results and WAV audio are in build\host\scenarios
pause
