# ============================================================
# GY 월드 오케스트레이터 — 서버 호스트 머신(EC2 등)에 상주.
#   worlds 큐(start_requested_at)를 감시해 GYServer.exe 를 월드당 프로세스로 스폰/회수한다.
#   실행:  powershell -ExecutionPolicy Bypass -File orchestrator.ps1
#   전제 (머신 env, setx /M 권장):
#     GY_HOSTED_URL          Supabase URL
#     GY_HOSTED_SECRET_KEY   service_role 키 (worlds RPC 권한)
#     GY_SERVER_EXE          GYServer.exe 전체 경로 (예: C:\GY\Server\GYServer.exe)
#     GY_PUBLIC_IP           이 머신의 공인 IP (EC2 = Elastic IP)
#   생명주기:
#     클라 request_world_start → claim_world_start(원자 클레임) → 스폰(-WorldId -port -PublicAddr)
#     → 서버 자체 하트비트가 online 전환 → 프로세스 종료 감지/유휴 리핑 → set_world_offline
# ============================================================
param(
    [string]$BaseUrl = $env:GY_HOSTED_URL,
    [string]$SecretKey = $env:GY_HOSTED_SECRET_KEY,
    [string]$ServerExe = $env:GY_SERVER_EXE,
    # 패키징 서버(GYServer.exe)는 비움. 에디터 exe 로 돌릴 때만 uproject 경로 + -server
    # (예: "C:\Unreal\GY\GY.uproject -server")
    [string]$ServerArgsPrefix = $env:GY_SERVER_ARGS_PREFIX,
    [string]$PublicIp = $env:GY_PUBLIC_IP,
    [int]$PortMin = 7777,
    [int]$PortMax = 7786,             # 포트 수 = 동시 월드 상한 (박스 램과 함께 packing 한도)
    [int]$PollSeconds = 5,
    [int]$IdleMinutes = 10,           # 인원 0 지속 시 회수
    [int]$StaleSeconds = 90           # 하트비트 무응답 = 죽은 세션
)

$ErrorActionPreference = "Stop"
if (-not $BaseUrl -or -not $SecretKey -or -not $ServerExe -or -not $PublicIp) {
    throw "missing env: GY_HOSTED_URL / GY_HOSTED_SECRET_KEY / GY_SERVER_EXE / GY_PUBLIC_IP"
}
if (-not (Test-Path $ServerExe)) { throw "server exe not found: $ServerExe" }

# 패키징 산출물 루트의 GYServer.exe 는 런처 스텁(실서버를 자식으로 스폰) — 스텁을 추적하면
# 램 측정/graceful 종료가 전부 헛돈다. 실제 바이너리가 있으면 그걸 직접 스폰
$realExe = Join-Path (Split-Path $ServerExe) "GY\Binaries\Win64\GYServer.exe"
if (Test-Path $realExe) { $ServerExe = $realExe }

$Headers = @{ "apikey" = $SecretKey; "Authorization" = "Bearer $SecretKey"; "Content-Type" = "application/json" }
# PowerShell 기본 UA 가 Mozilla/5.0 이라 호스티드 Supabase 가 브라우저로 오인 → secret key 요청을 403 차단. 명시 UA 필수
$UserAgent = "gy-orchestrator/1.0"

function Invoke-Rpc([string]$Name, [hashtable]$RpcParams) {
    $body = [System.Text.Encoding]::UTF8.GetBytes(($RpcParams | ConvertTo-Json -Compress))
    return Invoke-RestMethod -Uri "$BaseUrl/rest/v1/rpc/$Name" -Method Post -Headers $Headers -Body $body -TimeoutSec 10 -UserAgent $UserAgent
}
function Get-Worlds([string]$Filter) {
    return Invoke-RestMethod -Uri "$BaseUrl/rest/v1/worlds?$Filter" -Method Get -Headers $Headers -TimeoutSec 10 -UserAgent $UserAgent
}
function Log([string]$Message) {
    Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

# worldId → @{ Process; Port; IdleSince }
$Running = @{}

function Get-FreePort {
    $used = @($Running.Values | ForEach-Object { $_.Port })
    for ($p = $PortMin; $p -le $PortMax; $p++) { if ($used -notcontains $p) { return $p } }
    return $null
}

function Stop-World([long]$WorldId, [string]$Reason) {
    $entry = $Running[$WorldId]
    if (-not $entry) { return }
    Log "world ${WorldId}: stopping ($Reason)"
    if (-not $entry.Process.HasExited) {
        # taskkill (F 없이) = ConsoleCtrl → 서버의 종료 flush(월드+캐릭터 저장)가 돈다. 강제 킬 금지.
        # cmd /c 경유: PS5.1 은 네이티브 stderr 를 ErrorRecord 로 감싸 ErrorActionPreference=Stop 에서 스크립트를 죽인다
        cmd /c "taskkill /PID $($entry.Process.Id) >nul 2>&1"
        $entry.Process.WaitForExit(20000) | Out-Null
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
    $Running.Remove($WorldId)
    try { Invoke-Rpc "set_world_offline" @{ p_id = $WorldId } | Out-Null } catch { Log "set_world_offline($WorldId) failed: $($_.Exception.Message)" }
}

# ── 부팅 정리: 이전 에이전트 세대의 잔재(내가 모르는 online/starting 행) 오프라인 처리 ──
try {
    $stale = Get-Worlds "status=neq.offline&select=id,status"
    foreach ($w in $stale) {
        Log "boot cleanup: world $($w.id) was '$($w.status)' - marking offline"
        Invoke-Rpc "set_world_offline" @{ p_id = $w.id } | Out-Null
    }
} catch { Log "boot cleanup failed (continuing): $($_.Exception.Message)" }

Log "orchestrator up - exe=$ServerExe ip=$PublicIp ports=$PortMin-$PortMax poll=${PollSeconds}s"

while ($true) {
    Start-Sleep -Seconds $PollSeconds

    # ── 1. 죽은 프로세스 회수 (크래시/자체 종료) ──
    foreach ($worldId in @($Running.Keys)) {
        if ($Running[$worldId].Process.HasExited) {
            Log "world ${worldId}: process exited (code=$($Running[$worldId].Process.ExitCode))"
            $Running.Remove($worldId)
            try { Invoke-Rpc "set_world_offline" @{ p_id = $worldId } | Out-Null } catch {}
        }
    }

    # ── 2. 시작 요청 처리 ──
    try {
        $requested = Get-Worlds "status=eq.offline&start_requested_at=not.is.null&select=id,name"
    } catch { Log "poll failed: $($_.Exception.Message)"; continue }

    foreach ($w in $requested) {
        if ($Running.ContainsKey([long]$w.id)) { continue }
        $port = Get-FreePort
        if ($null -eq $port) { Log "world $($w.id): no free port - request deferred"; continue }

        # 원자 클레임 — 에이전트가 여럿이어도 한쪽만 스폰
        $claimed = $false
        try { $claimed = Invoke-Rpc "claim_world_start" @{ p_id = $w.id } } catch { Log "claim($($w.id)) failed: $($_.Exception.Message)" }
        if (-not $claimed) { continue }

        Log "world $($w.id): spawning on port $port"
        $spawnArgs = @()
        if ($ServerArgsPrefix) { $spawnArgs += ($ServerArgsPrefix -split " ") }
        # -nosteam: 패키징 서버는 Steam 이 타깃에서 제외돼 무의미(무해) — 에디터 exe 로 돌릴 때(ServerArgsPrefix)만 유효
        $spawnArgs += @("L_Expanse_WP", "-log", "-nosteam", "-port=$port", "-WorldId=$($w.id)", "-PublicAddr=${PublicIp}:$port")
        $proc = Start-Process -FilePath $ServerExe -ArgumentList $spawnArgs -PassThru -WindowStyle Minimized
        $Running[[long]$w.id] = @{ Process = $proc; Port = $port; IdleSince = $null }
    }

    # ── 3. 유휴/좀비 리핑 + 램 실측 로깅 ──
    try {
        $mine = Get-Worlds "select=id,status,player_count,heartbeat_at"
    } catch { continue }

    foreach ($worldId in @($Running.Keys)) {
        $row = $mine | Where-Object { [long]$_.id -eq $worldId }
        if (-not $row) { continue }
        $entry = $Running[$worldId]

        # 스폰했는데 하트비트가 안 옴 (부팅 실패/행) — starting 상태로 stale 이면 회수
        if ($row.status -eq "starting") {
            if (((Get-Date) - $entry.Process.StartTime).TotalSeconds -gt 300) { Stop-World $worldId "no heartbeat after spawn" }
            continue
        }

        # 인원 0 지속 → 회수 (종료 flush 가 마지막 저장을 보장)
        if ($row.player_count -eq 0) {
            if ($null -eq $entry.IdleSince) { $entry.IdleSince = Get-Date }
            elseif (((Get-Date) - $entry.IdleSince).TotalMinutes -ge $IdleMinutes) { Stop-World $worldId "idle ${IdleMinutes}m" }
        } else {
            $entry.IdleSince = $null
        }
    }

    # packing 한도 실측용 — 5분마다 프로세스별 램 로깅
    if ((Get-Date).Minute % 5 -eq 0 -and (Get-Date).Second -lt $PollSeconds) {
        foreach ($worldId in $Running.Keys) {
            $p = $Running[$worldId].Process
            if (-not $p.HasExited) {
                $p.Refresh()
                Log ("world {0}: RAM {1:N0} MB" -f $worldId, ($p.WorkingSet64 / 1MB))
            }
        }
    }
}
