$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$AssetDir = Join-Path $PSScriptRoot '..\..\Content\ThirdParty\PaidAssets'

function Select-Files
{
    param([string[]]$Files, [string]$Prompt)

    if (-not $Files -or $Files.Count -eq 0) { return @() }

    $input = Read-Host $Prompt
    $input = $input.Trim()

    if ($input -eq '' -or $input -eq 'none') { return @() }
    if ($input -eq 'all') { return $Files }

    $selected = @()
    foreach ($part in ($input -split ','))
    {
        $part = $part.Trim()
        if ($part -match '^(\d+)-(\d+)$')
        {
            $from = [int]$matches[1]
            $to   = [int]$matches[2]
            for ($i = $from; $i -le $to; $i++)
            {
                if ($i -ge 1 -and $i -le $Files.Count) { $selected += $Files[$i - 1] }
            }
        }
        elseif ($part -match '^\d+$')
        {
            $idx = [int]$part
            if ($idx -ge 1 -and $idx -le $Files.Count) { $selected += $Files[$idx - 1] }
        }
    }

    return $selected | Select-Object -Unique
}

Write-Host '드라이브와 비교 중...'

$incomingDryRun = cmd /c "rclone copy gdrive:GY_PaidAssets `"$AssetDir`" --dry-run 2>&1"
$incomingFiles = @($incomingDryRun |
    ForEach-Object {
        if ($_ -match 'NOTICE:\s+(.+?):\s+Skipped copy')
        {
            $matches[1]
        }
    })

if ($incomingFiles.Count -eq 0)
{
    Write-Host '다른 파일 없음.'
    Read-Host '엔터를 눌러 종료'
    exit
}

Write-Host ''
Write-Host "다른 파일 $($incomingFiles.Count)건:"
for ($i = 0; $i -lt $incomingFiles.Count; $i++)
{
    Write-Host "  [$($i + 1)] $($incomingFiles[$i])"
}
Write-Host ''

$toDownload = Select-Files -Files $incomingFiles -Prompt '다운로드할 파일 (예: 1,3 또는 1-3 또는 all 또는 none)'

if ($toDownload.Count -eq 0)
{
    Write-Host '취소됨.'
    Read-Host '엔터를 눌러 종료'
    exit
}

$tmpFile = [System.IO.Path]::GetTempFileName()
[System.IO.File]::WriteAllLines($tmpFile, $toDownload, [System.Text.UTF8Encoding]::new($false))

Write-Host ''
Write-Host '다운로드 중...'
rclone copy gdrive:GY_PaidAssets $AssetDir --files-from $tmpFile --progress

Remove-Item $tmpFile -Force -ErrorAction SilentlyContinue

Write-Host ''
Write-Host '완료!'
Read-Host '엔터를 눌러 종료'
