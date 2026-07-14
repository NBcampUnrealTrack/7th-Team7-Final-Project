# ============================================================
# GY 클라이언트 패키징 (Win64, Development — 패키징 빌드에서 콘솔 사용 가능)
#   실행: powershell -ExecutionPolicy Bypass -File scripts\package_client.ps1 [옵션]
#   prod(호스티드) 전용 — 백엔드 값은 파라미터 또는 환경변수(GY_HOSTED_*, CI Secrets) 필수
#   클라 산출물에는 SecretKey 를 절대 넣지 않는다 (빈 값 강제)
# ============================================================
param(
    [string]$ServerBaseUrl = $env:GY_HOSTED_URL,
    [string]$PublishableKey = $env:GY_HOSTED_PUBLISHABLE_KEY,
    [string]$AuthMode = "Steam",
    [string]$ArchiveDir = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Engine = "C:\Program Files\Epic Games\UE_5.7"
$IniPath = Join-Path $RepoRoot "Config\DefaultGYPersistence.ini"
$IniBackup = "$IniPath.prepackage.bak"
if (-not $ArchiveDir) { $ArchiveDir = Join-Path $RepoRoot "Build\Packaged" }

if (-not $ServerBaseUrl -or -not $PublishableKey) { throw "hosted ServerBaseUrl/PublishableKey required - pass params or set GY_HOSTED_URL / GY_HOSTED_PUBLISHABLE_KEY (CI secrets)" }

Write-Host "[package] backend=$ServerBaseUrl authMode=$AuthMode archive=$ArchiveDir"

# 개발 머신의 로컬 ini 는 백업 후 원복 (CI fresh checkout 은 없음 — gitignored)
$bHadLocalIni = Test-Path $IniPath
if ($bHadLocalIni) { Copy-Item $IniPath $IniBackup -Force }
try {
    # 패키징 순간에만 클라용 값으로 교체 — SecretKey 는 빈 값 (서버 전용 키, 클라 유출 금지)
    $iniText = "[/Script/GY.GYPersistenceSettings]`r`n" +
               "ServerBaseUrl=`"$ServerBaseUrl`"`r`n" +
               "SecretKey=`r`n" +
               "PublishableKey=$PublishableKey`r`n" +
               "AuthMode=$AuthMode`r`n" +
               "MockSteamId=dev_test`r`n"
    Set-Content -Path $IniPath -Value $iniText -Encoding ascii

    # -nocompileeditor: 쿡은 기존 에디터 바이너리 사용 (직전에 빌드돼 있어야 함).
    # MaxParallelActions=2: PCH 컴파일 메모리 피크 제한 — 병렬 3+에서 C3859(가상 메모리 부족) 발생 이력
    & "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
        -project="$RepoRoot\GY.uproject" `
        -platform=Win64 -clientconfig=Development `
        -build -nocompileeditor -cook -stage -pak -archive -archivedirectory="$ArchiveDir" `
        -UbtArgs="-MaxParallelActions=2" `
        -noP4 -utf8output -unattended
    if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed (exit $LASTEXITCODE)" }

    # SteamAPI 초기화용 — Steam 을 통해 실행되지 않는 배포본은 exe 옆에 이 파일이 필요
    $binDir = Join-Path $ArchiveDir "Windows\GY\Binaries\Win64"
    if (Test-Path $binDir) {
        Set-Content -Path (Join-Path $binDir "steam_appid.txt") -Value "480" -Encoding ascii -NoNewline
        Write-Host "[package] steam_appid.txt(480) written to $binDir"
    } else {
        Write-Host "[package] WARN: staged binaries dir not found: $binDir" -ForegroundColor Yellow
    }

    Write-Host "[package] Done: $ArchiveDir\Windows" -ForegroundColor Green
}
finally {
    # 개발 환경 ini 원복 — 실패해도 로컬 값이 남아있게 보장. CI(원본 없음)는 흔적 제거
    if ($bHadLocalIni) { Move-Item $IniBackup $IniPath -Force }
    else { Remove-Item $IniPath -Force -ErrorAction SilentlyContinue }
}
