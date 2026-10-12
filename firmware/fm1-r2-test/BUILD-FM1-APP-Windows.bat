@echo off
setlocal EnableExtensions
where wsl >nul 2>nul
if errorlevel 1 (
  echo WSL is required for the JieLi Linux pi32v2 toolchain.
  exit /b 2
)
if "%ETCH_FM1_CRT0%"=="" (
  echo Set ETCH_FM1_CRT0 to a verified AC791N/WL82 startup source or object.
  exit /b 2
)
if "%ETCH_FM1_LINKER_SCRIPT%"=="" (
  echo Set ETCH_FM1_LINKER_SCRIPT to a verified FM-1 linker script.
  exit /b 2
)
for /f "delims=" %%I in ('wsl wslpath -u "%ETCH_FM1_CRT0%"') do set "CRT0_WSL=%%I"
if "%CRT0_WSL%"=="" (
  echo Could not translate ETCH_FM1_CRT0 into a WSL path.
  exit /b 2
)
for /f "delims=" %%I in ('wsl wslpath -u "%ETCH_FM1_LINKER_SCRIPT%"') do set "LINKER_WSL=%%I"
if "%LINKER_WSL%"=="" (
  echo Could not translate ETCH_FM1_LINKER_SCRIPT into a WSL path.
  exit /b 2
)
wsl bash -lc "p=$(wslpath -u '%~dp0'); cd \"$p\" && ETCH_FM1_CRT0='%CRT0_WSL%' ETCH_FM1_LINKER_SCRIPT='%LINKER_WSL%' ./build_target_app.sh"
exit /b %errorlevel%
