# ============================================================
# GY 데디케이티드 서버 패키징 (Win64, Development)
#   ※ 소스 빌드 엔진 필수 — 런처(바이너리) 엔진은 Server 타깃 빌드 불가
#   실행 예:
#     powershell -ExecutionPolicy Bypass -File scripts\package_server.ps1 `
#       -EngineRoot "C:\UnrealEngine" `
#       -ServerBaseUrl "https://ajhrjxbumnicfspdfvna.supabase.co" `
#       -SecretKey "sb_secret_..."     (Supabase 대시보드 → Settings → API Keys)
#   -SecretKey 를 주면 산출물에 구움 — 배포는 서버 전용 프라이빗 itch 프로젝트로만
#     (claimed 다운로드 키, 클라 프로젝트와 분리. 산출물에서 키 추출 가능하므로 공개 금지)
#   생략하면 빈 값으로 굽고, 서버가 시작 시 GY_HOSTED_SECRET_KEY 환경변수에서 읽는다
# ============================================================
param(
    [string]$EngineRoot = $env:GY_SRC_ENGINE,
    [string]$ServerBaseUrl = $env:GY_HOSTED_URL,
    [string]$SecretKey = $env:GY_HOSTED_SECRET_KEY,
    [ValidateSet("Development", "Shipping")]
    [string]$Config = "Development",
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
if (-not $ServerBaseUrl) {
    throw "hosted ServerBaseUrl required - pass param or set GY_HOSTED_URL"
}

Write-Host "[server] engine=$EngineRoot backend=$ServerBaseUrl archive=$ArchiveDir"

$bHadLocalIni = Test-Path $IniPath
if ($bHadLocalIni) { Copy-Item $IniPath $IniBackup -Force }
try {
    if (-not $SecretKey) { Write-Host "[server] SecretKey not baked - server will read GY_HOSTED_SECRET_KEY env at runtime" -ForegroundColor Yellow }
    $iniText = "[/Script/GY.GYPersistenceSettings]`r`n" +
               "ServerBaseUrl=`"$ServerBaseUrl`"`r`n" +
               "SecretKey=$SecretKey`r`n"
    Set-Content -Path $IniPath -Value $iniText -Encoding ascii

    # MaxParallelActions=2: PCH 컴파일 메모리 피크 제한 — 병렬 3+에서 C1076/C3859(가상 메모리 부족) 발생 이력
    & "$EngineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
        -project="$RepoRoot\GY.uproject" `
        -platform=Win64 -serverconfig="$Config" `
        -server -noclient `
        -build -cook -stage -pak -archive -archivedirectory="$ArchiveDir" `
        -UbtArgs="-MaxParallelActions=2" `
        -noP4 -utf8output -unattended
    if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed (exit $LASTEXITCODE)" }

    Write-Host "[server] Done: $ArchiveDir\WindowsServer" -ForegroundColor Green
    Write-Host "[server] run: GYServer.exe L_TestMap -log -port=7777" -ForegroundColor Green
}
finally {
    if ($bHadLocalIni) { Move-Item $IniBackup $IniPath -Force }
    else { Remove-Item $IniPath -Force -ErrorAction SilentlyContinue }
}
