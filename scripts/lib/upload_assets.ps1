﻿$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$AssetDir     = Join-Path $PSScriptRoot '..\..\Content\ThirdParty\PaidAssets'
$ManifestPath = Join-Path $PSScriptRoot '..\asset_manifest.json'

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

$outgoingDryRun = cmd /c "rclone copy `"$AssetDir`" gdrive:GY_PaidAssets --dry-run --fast-list 2>&1"
$outgoingFiles = @($outgoingDryRun |
    ForEach-Object {
        if ($_ -match 'NOTICE:\s+(.+?):\s+Skipped copy')
        {
            $matches[1]
        }
    })

if ($outgoingFiles.Count -eq 0)
{
    Write-Host '다른 파일 없음.'
    Read-Host '엔터를 눌러 종료'
    exit
}

Write-Host ''
Write-Host "다른 파일 $($outgoingFiles.Count)건:"
for ($i = 0; $i -lt $outgoingFiles.Count; $i++)
{
    Write-Host "  [$($i + 1)] $($outgoingFiles[$i])"
}
Write-Host ''

$toUpload = Select-Files -Files $outgoingFiles -Prompt '업로드할 파일 (예: 1,3 또는 1-3 또는 all 또는 none)'

if ($toUpload.Count -eq 0)
{
    Write-Host '취소됨.'
    Read-Host '엔터를 눌러 종료'
    exit
}

$notes = @{}
if ($toUpload.Count -eq 1)
{
    $notes[$toUpload[0]] = Read-Host '변경 노트'
}
else
{
    Write-Host ''
    Write-Host '각 파일별 노트 (엔터로 건너뛰기 가능):'
    for ($i = 0; $i -lt $toUpload.Count; $i++)
    {
        $f = $toUpload[$i]
        Write-Host ''
        Write-Host "  [$($i + 1)/$($toUpload.Count)] $f"
        $notes[$f] = Read-Host '  노트'
    }
}

$tmpFile = [System.IO.Path]::GetTempFileName()
[System.IO.File]::WriteAllLines($tmpFile, $toUpload, [System.Text.UTF8Encoding]::new($false))

Write-Host ''
Write-Host '업로드 중...'
rclone copy $AssetDir gdrive:GY_PaidAssets --files-from $tmpFile --fast-list --progress

Remove-Item $tmpFile -Force -ErrorAction SilentlyContinue

$datetime = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$user     = $env:USERNAME

if (Test-Path $ManifestPath)
{
    $manifest = Get-Content $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
}
else
{
    $manifest = [pscustomobject]@{ assets = [pscustomobject]@{} }
}

$assetsMap = [ordered]@{}
if ($manifest.assets)
{
    foreach ($prop in $manifest.assets.PSObject.Properties)
    {
        $assetsMap[$prop.Name] = $prop.Value
    }
}

foreach ($file in $toUpload)
{
    $historyItem = [pscustomobject]@{
        by   = $user
        at   = $datetime
        note = $notes[$file]
    }

    if ($assetsMap.Contains($file))
    {
        $existing = $assetsMap[$file]
        $existing.history = @($existing.history) + $historyItem
    }
    else
    {
        $assetsMap[$file] = [pscustomobject]@{ history = @($historyItem) }
    }
}

$sortedAssets = [ordered]@{}
foreach ($key in ($assetsMap.Keys | Sort-Object))
{
    $sortedAssets[$key] = $assetsMap[$key]
}
$manifest.assets = $sortedAssets

$manifest | ConvertTo-Json -Depth 10 | Set-Content $ManifestPath -Encoding UTF8

Write-Host ''
Write-Host '완료! asset_manifest.json 변경 사항 확인하고 커밋해주세요.'
Read-Host '엔터를 눌러 종료'
