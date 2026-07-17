# ============================================================
# GY 서버 배포 워처 — EC2 상주 (예약 작업으로 1분 주기 실행).
#   S3 의 latest.json 버전이 로컬과 다르면: 서버 zip 다운로드 → 오케스트레이터/서버 정지
#   → 폴더 교체 → 오케스트레이터 재시작. 박스가 꺼져 있던 동안의 배포도 부팅 후 첫 폴링에서 따라잡는다.
#
#   전제 (EC2 1회 세팅):
#     - 인스턴스 프로파일에 s3:GetObject (배포 버킷) — AWS 자격증명 파일 불필요
#     - 예약 작업 등록 (관리자 PowerShell):
#         schtasks /Create /TN GYDeployWatcher /SC MINUTE /MO 1 /RU SYSTEM /TR `
#           "powershell -NoProfile -ExecutionPolicy Bypass -File C:\GY\ec2_deploy_watcher.ps1"
#     - 오케스트레이터도 예약 작업으로 (워처가 재시작을 이 이름으로 건다):
#         schtasks /Create /TN GYOrchestrator /SC ONSTART /RU SYSTEM /TR `
#           "powershell -NoProfile -ExecutionPolicy Bypass -File C:\GY\orchestrator.ps1"
#
#   활성 플레이어가 있으면 배포를 미룬다 (다음 폴링에서 재시도) — 강제하려면 -Force
# ============================================================
param(
    [string]$Bucket = $env:GY_DEPLOY_BUCKET,
    [string]$Region = "ap-northeast-2",
    [string]$InstallDir = "C:\GY\Server",
    [string]$StateFile = "C:\GY\deploy_version.txt",
    [string]$LogFile = "C:\GY\deploy_watcher.log",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Log([string]$Message) {
    $line = "[{0}] {1}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Message
    Write-Host $line
    Add-Content -Path $LogFile -Value $line
}

if (-not $Bucket) { Log "GY_DEPLOY_BUCKET env missing - abort"; exit 1 }

Import-Module AWSPowerShell -ErrorAction SilentlyContinue

# 1. 원격 버전 확인
$tempDir = Join-Path $env:TEMP "gy_deploy"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
$latestPath = Join-Path $tempDir "latest.json"
try {
    Read-S3Object -BucketName $Bucket -Key "server/latest.json" -File $latestPath -Region $Region | Out-Null
} catch {
    Log "latest.json fetch failed: $($_.Exception.Message)"; exit 1
}
$latest = Get-Content $latestPath -Raw | ConvertFrom-Json
$remoteVersion = $latest.version
$zipKey = $latest.key

$localVersion = if (Test-Path $StateFile) { (Get-Content $StateFile -Raw).Trim() } else { "" }
if ($remoteVersion -eq $localVersion) { exit 0 }

Log "new version: $remoteVersion (local: $localVersion)"

# 2. 활성 플레이어 확인 — 있으면 미룬다 (오케스트레이터가 쓰는 Supabase 환경 그대로)
if (-not $Force -and $env:GY_HOSTED_URL -and $env:GY_HOSTED_SECRET_KEY) {
    try {
        $headers = @{ "apikey" = $env:GY_HOSTED_SECRET_KEY; "Authorization" = "Bearer $($env:GY_HOSTED_SECRET_KEY)" }
        $worlds = Invoke-RestMethod -Uri "$($env:GY_HOSTED_URL)/rest/v1/worlds?status=eq.online&select=player_count" `
            -Headers $headers -TimeoutSec 10 -UserAgent "gy-deploy-watcher/1.0"
        $players = ($worlds | Measure-Object -Property player_count -Sum).Sum
        if ($players -gt 0) { Log "deploy deferred - $players player(s) online"; exit 0 }
    } catch {
        Log "player check failed (deploying anyway): $($_.Exception.Message)"
    }
}

# 3. zip 다운로드
$zipPath = Join-Path $tempDir "server.zip"
Log "downloading s3://$Bucket/$zipKey"
Read-S3Object -BucketName $Bucket -Key $zipKey -File $zipPath -Region $Region | Out-Null

# 4. 오케스트레이터 + 서버 프로세스 정지 (서버는 EndPlay 에서 set_world_offline 을 쏜다)
# 프로세스명: Development = GYServer, Shipping = GYServer-Win64-Shipping
schtasks /End /TN GYOrchestrator 2>$null | Out-Null
Get-Process "GYServer*" -ErrorAction SilentlyContinue | ForEach-Object {
    Log "stopping server pid $($_.Id)"
    $_.CloseMainWindow() | Out-Null
}
Start-Sleep -Seconds 5
Get-Process "GYServer*" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

# 5. 폴더 교체 (이전 버전은 .prev 로 1개 보존 — 롤백용)
$prevDir = "$InstallDir.prev"
if (Test-Path $prevDir) { Remove-Item -Recurse -Force $prevDir }
if (Test-Path $InstallDir) { Move-Item $InstallDir $prevDir }
Expand-Archive $zipPath -DestinationPath $InstallDir -Force
Remove-Item $zipPath -Force

Set-Content -Path $StateFile -Value $remoteVersion

# 6. 오케스트레이터 재시작
schtasks /Run /TN GYOrchestrator | Out-Null
Log "deployed $remoteVersion and restarted orchestrator"
