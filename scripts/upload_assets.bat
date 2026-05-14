echo Checking changes...

rclone copy .\..\Content\ThirdParty\PaidAssets gdrive:GY_PaidAssets --dry-run 2>&1 | findstr "Skipped copy" > changed_files.tmp

powershell -Command "
  $lines = Get-Content 'changed_files.tmp';
  if ($lines.Count -eq 0) {
    Write-Host 'No changes found.';
    exit;
  }
  Write-Host 'Changed files:';
  $files = @();
  foreach ($line in $lines) {
    $file = ($line -split 'Skipped copy')[1].Trim();
    $files += $file;
    Write-Host '  -' $file;
  }
  $files | ConvertTo-Json | Set-Content 'changed_files.tmp';
"

if not exist changed_files.tmp (
    pause
    exit /b
)

set /p CONFIRM=Upload these files? (y/n): 
if /i "%CONFIRM%" neq "y" (
    del changed_files.tmp
    exit /b
)

set /p NOTE=Note: 

echo.
echo Uploading...
rclone copy .\..\Content\ThirdParty\PaidAssets gdrive:GY_PaidAssets --progress

powershell -Command "
  $date = Get-Date -Format 'yyyy-MM-dd';
  $user = $env:USERNAME;
  $files = Get-Content 'changed_files.tmp' | ConvertFrom-Json;
  $m = Get-Content 'asset_manifest.json' | ConvertFrom-Json;

  foreach ($file in $files) {
    $entry = $m.assets | Where-Object { $_.name -eq $file };
    if ($entry) {
      $entry.modified_by = $user;
      $entry.modified = $date;
      $entry.note = '%NOTE%';
    } else {
      $m.assets += [pscustomobject]@{name=$file; modified_by=$user; modified=$date; note='%NOTE%'};
    }
  }
  $m | ConvertTo-Json -Depth 3 | Set-Content 'asset_manifest.json';
"

del changed_files.tmp

echo.
echo Done! Please git push.
pause

