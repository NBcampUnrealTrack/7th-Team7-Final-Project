@echo off
setlocal
rem Start local Supabase stack.
"%~dp0.bin\supabase.exe" start
echo.
echo [done] Studio: http://127.0.0.1:54323   API: http://127.0.0.1:54321
echo Press any key to close.
pause >nul
