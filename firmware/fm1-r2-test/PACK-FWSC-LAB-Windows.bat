@echo off
setlocal
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
if "%~3"=="" goto :usage
python "%~dp0..\..\tools\fm1_fwsc.py" lab-pack "%~1" "%~2" "%~3" --sdk "%AC79_SDK%"
exit /b %errorlevel%
:usage
echo Usage: PACK-FWSC-LAB-Windows.bat APP.bin OTA.bin OUT.fwsc.lab
echo Set AC79_SDK to the JieLi AC79 SDK checkout first.
exit /b 2
