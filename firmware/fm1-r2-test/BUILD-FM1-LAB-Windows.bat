@echo off
setlocal EnableExtensions
if "%~1"=="" goto :usage
if "%~2"=="" goto :usage
call "%~dp0BUILD-FM1-APP-Windows.bat"
if errorlevel 1 exit /b %errorlevel%
set "APP=%~dp0..\..\build\fm1-r2-test\app.bin"
if not exist "%APP%" (
  echo Audited app.bin was not produced: %APP%
  exit /b 2
)
call "%~dp0PACK-FROM-STOCK-LAB-Windows.bat" "%APP%" "%~1" "%~2"
exit /b %errorlevel%
:usage
echo Usage: BUILD-FM1-LAB-Windows.bat OFFICIAL-FM1.fwsc OUT.fwsc.lab
echo Requires ETCH_FM1_CRT0, ETCH_FM1_LINKER_SCRIPT and verified fm1_sdk_bridge.cpp.
exit /b 2
