@echo off
setlocal
rem Show local Supabase status (URLs, keys, running state).
"%~dp0.bin\supabase.exe" status
echo.
echo Press any key to close.
pause >nul
