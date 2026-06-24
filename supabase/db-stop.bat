@echo off
setlocal
rem Stop local Supabase stack (data is kept).
"%~dp0.bin\supabase.exe" stop
echo.
echo [done] Press any key to close.
pause >nul
