echo Watching asset folder...
echo Press Ctrl+C to stop.

powershell -Command "
  $watcher = New-Object System.IO.FileSystemWatcher;
  $watcher.Path = (Resolve-Path '.\..\Content\ThirdParty\PaidAssets');
  $watcher.IncludeSubdirectories = $true;
  $watcher.EnableRaisingEvents = $true;

  $action = {
    $path = $Event.SourceEventArgs.FullPath;
    $name = Split-Path $path -Leaf;
    $time = Get-Date -Format 'HH:mm:ss';

    Add-Type -AssemblyName System.Windows.Forms;
    [System.Windows.Forms.MessageBox]::Show(
      ($time + ' - ' + $name + ' changed. Please run upload_assets.bat'),
      'Asset Changed'
    );
  };

  Register-ObjectEvent $watcher Changed -Action $action | Out-Null;
  Register-ObjectEvent $watcher Created -Action $action | Out-Null;

  while ($true) { Start-Sleep 1 }
"

