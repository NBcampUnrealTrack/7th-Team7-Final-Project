@echo off
setlocal

rem Dedicated server launch (binary engine: editor exe -server)
rem Usage: run_server.bat [Map] [Port]
rem   ex) run_server.bat L_Expanse_WP 7777
rem Override engine path with UE_ENGINE env var if needed.

set "PROJECT_ROOT=%~dp0.."
set "UPROJECT=%PROJECT_ROOT%\GY.uproject"

set "MAP=%~1"
if "%MAP%"=="" set "MAP=L_Expanse_WP"

set "PORT=%~2"
if "%PORT%"=="" set "PORT=7777"

set "UE=%UE_ENGINE%"
if "%UE%"=="" set "UE=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%UE%" (
    echo [!] UnrealEditor.exe not found: %UE%
    echo     Set UE_ENGINE env var to override.
    pause
    exit /b 1
)

echo ============================================
echo  GY Dedicated Server ^(editor -server^)
echo  Map : %MAP%
echo  Port: %PORT%
echo ============================================

rem -nosteam: 데디는 Steam 불필요 - Shipping에서 Steam SDK DLL 로드 assert 방지
"%UE%" "%UPROJECT%" %MAP% -server -log -nosteam -port=%PORT%

pause
