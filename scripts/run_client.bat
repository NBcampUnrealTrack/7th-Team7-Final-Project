@echo off
setlocal

rem Client launch
rem   no arg  : boot menu level (L_MainMenu_Temp), type IP in the widget
rem   with IP : connect directly to the dedicated server (test shortcut)
rem Usage: run_client.bat [ServerIP] [Port]
rem   ex) run_client.bat
rem       run_client.bat 203.0.113.5
rem       run_client.bat 203.0.113.5 7777
rem Override engine path with UE_ENGINE env var if needed.

set "PROJECT_ROOT=%~dp0.."
set "UPROJECT=%PROJECT_ROOT%\GY.uproject"

set "UE=%UE_ENGINE%"
if "%UE%"=="" set "UE=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%UE%" (
    echo [!] UnrealEditor.exe not found: %UE%
    echo     Set UE_ENGINE env var to override.
    pause
    exit /b 1
)

set "SERVER_IP=%~1"
set "PORT=%~2"
if "%PORT%"=="" set "PORT=7777"

if "%SERVER_IP%"=="" (
    echo Booting menu level L_MainMenu_Temp ...
    "%UE%" "%UPROJECT%" L_MainMenu_Temp -game -log
) else (
    echo Connecting to %SERVER_IP%:%PORT% ...
    "%UE%" "%UPROJECT%" %SERVER_IP%:%PORT% -game -log
)

pause
