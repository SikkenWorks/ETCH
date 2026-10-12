@echo off
setlocal
where wsl >nul 2>&1 || (echo WSL with Ubuntu is required. & exit /b 1)
for /f "delims=" %%P in ('wsl wslpath -a "%~dp0.."') do set "ETCH_LINUX=%%P"
if not defined ETCH_LINUX (echo Cannot resolve ETCH directory in WSL. & exit /b 1)
wsl bash -lc "cd '%ETCH_LINUX%' && python3 tools/run_native_tests.py --self-test-hang && python3 tools/run_native_tests.py --timeout 35 --jobs 3 --report build/native-regression-v057.json"
if errorlevel 1 (echo Native tests FAILED. & exit /b 1)
echo All native tests PASS.
endlocal
