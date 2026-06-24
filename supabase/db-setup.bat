@echo off
setlocal
rem Double-click to run db-setup.ps1 (bypass policy, keep window open).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0db-setup.ps1"
echo.
echo [done] Press any key to close.
pause >nul
