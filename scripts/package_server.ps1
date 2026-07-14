# ============================================================
# GY 데디케이티드 서버 패키징 (Win64, Development)
#   ※ 소스 빌드 엔진 필수 — 런처(바이너리) 엔진은 Server 타깃 빌드 불가
#   실행 예:
#     powershell -ExecutionPolicy Bypass -File scripts\package_server.ps1 `
#       -EngineRoot "C:\UnrealEngine" `
#       -ServerBaseUrl "https://ajhrjxbumnicfspdfvna.supabase.co" `
#       -SecretKey "sb_secret_..."     (Supabase 대시보드 → Settings → API Keys)
#   서버 산출물에는 SecretKey 가 들어간다(저장 경로 권위) — 산출물을 팀 밖에 공유 금지
# ============================================================
param(
    [string]$EngineRoot = $env:GY_SRC_ENGINE,
    [string]$ServerBaseUrl = $env:GY_HOSTED_URL,
    [string]$SecretKey = $env:GY_HOSTED_SECRET_KEY,
    [string]$ArchiveDir = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$IniPath = Join-Path $RepoRoot "Config\DefaultGYPersistence.ini"
$IniBackup = "$IniPath.prepackage.bak"
if (-not $ArchiveDir) { $ArchiveDir = Join-Path $RepoRoot "Build\PackagedServer" }

if (-not $EngineRoot -or -not (Test-Path "$EngineRoot\Engine\Build\BatchFiles\RunUAT.bat")) {
    throw "source engine not found - pass -EngineRoot or set GY_SRC_ENGINE (launcher engine cannot build Server targets)"
}
if (-not $ServerBaseUrl -or -not $SecretKey) {
    throw "hosted ServerBaseUrl/SecretKey required - pass params or set GY_HOSTED_URL / GY_HOSTED_SECRET_KEY"
}

Write-Host "[server] engine=$EngineRoot backend=$ServerBaseUrl archive=$ArchiveDir"

$bHadLocalIni = Test-Path $IniPath
if ($bHadLocalIni) { Copy-Item $IniPath $IniBackup -Force }
try {
    # 서버는 SecretKey 필수(저장 권위), 클라용 필드는 서버에서 안 쓰므로 생략
    $iniText = "[/Script/GY.GYPersistenceSettings]`r`n" +
               "ServerBaseUrl=`"$ServerBaseUrl`"`r`n" +
               "SecretKey=$SecretKey`r`n"
    Set-Content -Path $IniPath -Value $iniText -Encoding ascii

    & "$EngineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
        -project="$RepoRoot\GY.uproject" `
        -platform=Win64 -serverconfig=Development `
        -server -noclient `
        -build -cook -stage -pak -archive -archivedirectory="$ArchiveDir" `
        -noP4 -utf8output -unattended
    if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed (exit $LASTEXITCODE)" }

    Write-Host "[server] Done: $ArchiveDir\WindowsServer" -ForegroundColor Green
    Write-Host "[server] run: GYServer.exe L_TestMap -log -port=7777" -ForegroundColor Green
}
finally {
    if ($bHadLocalIni) { Move-Item $IniBackup $IniPath -Force }
    else { Remove-Item $IniPath -Force -ErrorAction SilentlyContinue }
}
