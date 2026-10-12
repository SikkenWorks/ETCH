@echo off
setlocal
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
python "%~dp0..\..\tools\fm1_fwsc.py" extract "%~1" "%~2"
exit /b %errorlevel%
:usage
echo Usage: EXTRACT-STOCK-Windows.bat OFFICIAL-FM1.fwsc OUT-DIRECTORY
exit /b 2
