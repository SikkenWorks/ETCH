@echo off
setlocal
where wsl >nul 2>nul
if errorlevel 1 (
  echo WSL is required for the JieLi Linux pi32v2 toolchain.
  exit /b 2
)
wsl bash -lc "p=$(wslpath -u '%~dp0'); cd \"$p\" && ./build_target_objects.sh"
endlocal
