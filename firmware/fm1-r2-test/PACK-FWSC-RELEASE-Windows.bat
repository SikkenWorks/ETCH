@echo off
setlocal
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
if "%~3"=="" goto :usage
python "%~dp0..\..\tools\fm1_fwsc.py" release-pack "%~1" "%~2" "%~3" --sdk "%AC79_SDK%" --proof "%~dp0..\..\R0_RECOVERY_PROVEN.json"
exit /b %errorlevel%
:usage
echo Usage: PACK-FWSC-RELEASE-Windows.bat APP.bin OTA.bin OUT.fwsc
echo Set AC79_SDK to the JieLi AC79 SDK checkout first.
echo This command refuses to create a .fwsc until R0_RECOVERY_PROVEN.json passes the ETCH gate.
exit /b 2
