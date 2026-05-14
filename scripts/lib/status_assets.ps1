param([switch]$NoPrompt)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$AssetDir = Join-Path $PSScriptRoot '..\..\Content\ThirdParty\PaidAssets'

Write-Host '동기화 상태 확인 중...'

$dryRun = cmd /c "rclone copy `"$AssetDir`" gdrive:GY_PaidAssets --dry-run 2>&1"
$changedFiles = $dryRun |
    ForEach-Object {
        if ($_ -match 'NOTICE:\s+(.+?):\s+Skipped copy')
        {
            $matches[1]
        }
    }

if (-not $changedFiles -or $changedFiles.Count -eq 0)
{
    Write-Host ''
    Write-Host '모두 동기화됨.'
}
else
{
    Write-Host ''
    Write-Host "동기화 안 된 파일 $($changedFiles.Count)건:"
    foreach ($f in $changedFiles)
    {
        Write-Host "  - $f"
    }
    Write-Host ''
    Write-Host '업로드: scripts\upload_assets.bat'
    Write-Host '다운로드: scripts\sync_assets.bat'
}

if (-not $NoPrompt)
{
    Read-Host '엔터를 눌러 종료'
}
