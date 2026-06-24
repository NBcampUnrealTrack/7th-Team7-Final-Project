@echo off
setlocal
rem Reset local Supabase DB: re-apply migrations + seed from scratch.
"%~dp0.bin\supabase.exe" db reset
echo.
echo [done] Press any key to close.
pause >nul
