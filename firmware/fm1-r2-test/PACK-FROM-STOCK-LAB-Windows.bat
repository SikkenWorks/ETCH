@echo off
setlocal
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
if "%~3"=="" goto :usage
python "%~dp0..\..\tools\fm1_fwsc.py" lab-pack-from-stock "%~1" "%~2" "%~3"
exit /b %errorlevel%
:usage
echo Usage: PACK-FROM-STOCK-LAB-Windows.bat APP.bin OFFICIAL-FM1.fwsc OUT.fwsc.lab
exit /b 2
