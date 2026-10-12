@echo off
setlocal
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
if "%~3"=="" goto :usage
python "%~dp0..\..\tools\fm1_fwsc.py" release-pack-from-stock "%~1" "%~2" "%~3" --proof "%~dp0..\..\R0_RECOVERY_PROVEN.json"
exit /b %errorlevel%
:usage
echo Usage: PACK-FROM-STOCK-RELEASE-Windows.bat APP.bin OFFICIAL-FM1.fwsc OUT.fwsc
echo Release output remains blocked until R0_RECOVERY_PROVEN.json passes.
exit /b 2
