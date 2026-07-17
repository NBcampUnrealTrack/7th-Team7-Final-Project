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
    [int]$IdleMinutes = 10,           # 인원 0 지속 시 회수 — 짧게 유지해 슬롯 회전 (콜드 스타트 해법은 웜 풀)
    [int]$StaleSeconds = 90,          # 하트비트 무응답 = 죽은 세션
    # 상시 가동 월드 (쉼표 구분 id) — 부팅 시 자동 스폰 + 유휴 회수 제외.
    # 주의: MaxWorlds=1 운영에선 핀 월드가 유일한 슬롯을 영구 점유해 다른 월드가 영원히 대기 —
    # 기본 비움. 박스를 키우거나 MaxWorlds 를 올릴 때만 팀 메인 월드를 핀할 것
    [string]$AlwaysOnWorlds = "",
    # 웜 스탠바이 수 — 맵까지 부팅한 채 월드 배정을 기다리는 예비 서버 (배정 소비 시 자동 보충).
    # 0 = 비활성(콜드 스폰만). 배정 시 입장 대기가 분 단위 → 초 단위로 준다
    [int]$StandbyCount = 0,
    # 동시 활성 월드 상한 — 박스 CPU 보호 (m7i-flex 2vCPU 실측: 활성 1개가 안전).
    # 초과 요청은 스폰하지 않고 대기 — 클라는 "대기열" 표시, 슬롯이 비면(유휴 회수) 자동 처리
    [int]$MaxWorlds = 1
)

$ErrorActionPreference = "Stop"
if (-not $BaseUrl -or -not $SecretKey -or -not $ServerExe -or -not $PublicIp) {
    throw "missing env: GY_HOSTED_URL / GY_HOSTED_SECRET_KEY / GY_SERVER_EXE / GY_PUBLIC_IP"
}
if (-not (Test-Path $ServerExe)) { throw "server exe not found: $ServerExe" }

# 패키징 산출물 루트의 GYServer.exe 는 런처 스텁(실서버를 자식으로 스폰) — 스텁을 추적하면
# 램 측정/graceful 종료가 전부 헛돈다. 실제 바이너리가 있으면 그걸 직접 스폰 (Shipping 은 이름이 다름)
foreach ($candidate in @("GYServer.exe", "GYServer-Win64-Shipping.exe")) {
    $realExe = Join-Path (Split-Path $ServerExe) "GY\Binaries\Win64\$candidate"
    if (Test-Path $realExe) { $ServerExe = $realExe; break }
}

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
    $line = "[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message
    Write-Host $line
    # 예약 작업(SYSTEM, 콘솔 없음) 실행 대비 파일 로그 — 스크립트 옆에 쌓인다 (EC2 = C:\GY\)
    try { Add-Content -Path (Join-Path $PSScriptRoot "orchestrator.log") -Value $line } catch {}
}

# worldId → @{ Process; Port; IdleSince }
$Running = @{}
# 스탠바이 프로세스 목록: @{ Process; Port; SpawnedAt } — 배정 소비 시 $Running 으로 이동
$StandbyProcs = New-Object System.Collections.ArrayList
$PinnedWorlds = @()
if ($AlwaysOnWorlds) { $PinnedWorlds = @($AlwaysOnWorlds -split "," | ForEach-Object { [long]$_.Trim() }) }

function Get-Standbys {
    return Invoke-RestMethod -Uri "$BaseUrl/rest/v1/standby_servers?select=id,host_addr,assigned_world_id,heartbeat_at" -Method Get -Headers $Headers -TimeoutSec 10 -UserAgent $UserAgent
}

function Get-FreePort {
    $used = @($Running.Values | ForEach-Object { $_.Port }) + @($StandbyProcs | ForEach-Object { $_.Port })
    for ($p = $PortMin; $p -le $PortMax; $p++) { if ($used -notcontains $p) { return $p } }
    return $null
}

function Start-StandbyServer {
    $port = Get-FreePort
    if ($null -eq $port) { return }

    Log "standby: spawning on port $port"
    $spawnArgs = @()
    if ($ServerArgsPrefix) { $spawnArgs += ($ServerArgsPrefix -split " ") }
    $spawnArgs += @("L_Expanse_WP", "-log", "-nosteam", "-Standby", "-port=$port", "-PublicAddr=${PublicIp}:$port")
    $proc = Start-Process -FilePath $ServerExe -ArgumentList $spawnArgs -PassThru -WindowStyle Minimized
    [void]$StandbyProcs.Add(@{ Process = $proc; Port = $port; SpawnedAt = Get-Date })
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

# ── 부팅 정리: 이전 에이전트 세대의 잔재(내가 모르는 online/starting 행 + 스탠바이 행) 정리 ──
try {
    $stale = Get-Worlds "status=neq.offline&select=id,status"
    foreach ($w in $stale) {
        Log "boot cleanup: world $($w.id) was '$($w.status)' - marking offline"
        Invoke-Rpc "set_world_offline" @{ p_id = $w.id } | Out-Null
    }
    foreach ($s in @(Get-Standbys)) {
        Log "boot cleanup: standby $($s.id) - removing"
        Invoke-Rpc "consume_standby" @{ p_id = $s.id } | Out-Null
    }
} catch { Log "boot cleanup failed (continuing): $($_.Exception.Message)" }

Log "orchestrator up - exe=$ServerExe ip=$PublicIp ports=$PortMin-$PortMax poll=${PollSeconds}s pinned=[$($PinnedWorlds -join ',')]"

# 상시 가동 월드는 부팅하자마자 시작 요청을 스스로 큐잉 (다음 폴에서 스폰)
foreach ($pinnedId in $PinnedWorlds) {
    try { Invoke-Rpc "request_world_start" @{ p_id = $pinnedId } | Out-Null } catch { Log "pinned world $pinnedId request failed: $($_.Exception.Message)" }
}

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
    # 상시 가동 월드 자가 회복: 안 돌고 있으면 재요청 (RPC 가 offline 일 때만 큐잉하므로 무해)
    foreach ($pinnedId in $PinnedWorlds) {
        if (-not $Running.ContainsKey($pinnedId)) {
            try { Invoke-Rpc "request_world_start" @{ p_id = $pinnedId } | Out-Null } catch {}
        }
    }

    try {
        $requested = Get-Worlds "status=eq.offline&start_requested_at=not.is.null&select=id,name"
    } catch { Log "poll failed: $($_.Exception.Message)"; continue }

    foreach ($w in $requested) {
        if ($Running.ContainsKey([long]$w.id)) { continue }

        # 활성 월드 상한 — 초과분은 스폰하지 않음 (요청 타임스탬프가 남아 슬롯이 비면 다음 폴에서 처리)
        if ($Running.Count -ge $MaxWorlds) {
            Log "world $($w.id): queued (active $($Running.Count)/$MaxWorlds)"
            break
        }

        # 웜 스탠바이 우선 배정 (원자 RPC: worlds starting 전환 + standby 행에 월드 기록) —
        # 배정되면 스탠바이 서버가 폴링으로 감지해 세이브만 로드하고 online 전환 (수 초)
        $assignedStandby = $null
        try { $assignedStandby = Invoke-Rpc "assign_standby_to_world" @{ p_world_id = $w.id } } catch { Log "assign($($w.id)) failed: $($_.Exception.Message)"; continue }
        # JSON null 은 PS 에서 문자열 "null" 로 들어온다 — 숫자만 배정 성공으로
        if ("$assignedStandby" -notmatch '^\d+$') { $assignedStandby = $null }

        if ($assignedStandby) {
            # 스탠바이 프로세스를 월드 소유로 이관 (유휴 회수/램 로깅 대상이 되도록)
            $standbyRow = @(Get-Standbys) | Where-Object { [long]$_.id -eq [long]$assignedStandby } | Select-Object -First 1
            $standbyPort = if ($standbyRow) { [int]($standbyRow.host_addr -split ":")[-1] } else { 0 }
            $procEntry = @($StandbyProcs) | Where-Object { $_.Port -eq $standbyPort } | Select-Object -First 1
            if ($procEntry) {
                $StandbyProcs.Remove($procEntry)
                $Running[[long]$w.id] = @{ Process = $procEntry.Process; Port = $procEntry.Port; IdleSince = $null }
            }
            Log "world $($w.id): assigned to standby $assignedStandby (port $standbyPort)"
            continue
        }

        # 스탠바이 없음 — RPC 가 이미 starting 으로 클레임했으므로 바로 콜드 스폰
        $port = Get-FreePort
        if ($null -eq $port) { Log "world $($w.id): no free port - request deferred"; try { Invoke-Rpc "set_world_offline" @{ p_id = $w.id } | Out-Null } catch {}; continue }

        Log "world $($w.id): cold spawning on port $port"
        $spawnArgs = @()
        if ($ServerArgsPrefix) { $spawnArgs += ($ServerArgsPrefix -split " ") }
        # -nosteam: 패키징 서버는 Steam 이 타깃에서 제외돼 무의미(무해) — 에디터 exe 로 돌릴 때(ServerArgsPrefix)만 유효
        $spawnArgs += @("L_Expanse_WP", "-log", "-nosteam", "-port=$port", "-WorldId=$($w.id)", "-PublicAddr=${PublicIp}:$port")
        $proc = Start-Process -FilePath $ServerExe -ArgumentList $spawnArgs -PassThru -WindowStyle Minimized
        $Running[[long]$w.id] = @{ Process = $proc; Port = $port; IdleSince = $null }
    }

    # ── 2.5 스탠바이 풀 유지 ──
    # 죽은 스탠바이 프로세스 정리
    foreach ($entry in @($StandbyProcs)) {
        if ($entry.Process.HasExited) {
            Log "standby (port $($entry.Port)): process exited"
            $StandbyProcs.Remove($entry)
        }
    }
    # stale 스탠바이 행 리핑 (프로세스 급사로 행만 남은 경우)
    try {
        $staleIso = [DateTime]::UtcNow.AddSeconds(-$StaleSeconds).ToString("yyyy-MM-ddTHH:mm:ssZ")
        Invoke-RestMethod -Uri "$BaseUrl/rest/v1/standby_servers?heartbeat_at=lt.$staleIso&assigned_world_id=is.null" -Method Delete -Headers $Headers -TimeoutSec 10 -UserAgent $UserAgent | Out-Null
    } catch {}
    # 부족분 보충 (포트/램이 허락하는 한)
    while ($StandbyProcs.Count -lt $StandbyCount) {
        $before = $StandbyProcs.Count
        Start-StandbyServer
        if ($StandbyProcs.Count -eq $before) { break } # 포트 소진 등 — 다음 폴에서 재시도
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

        # 상시 가동 월드는 유휴 회수 제외
        if ($PinnedWorlds -contains $worldId) { continue }

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
