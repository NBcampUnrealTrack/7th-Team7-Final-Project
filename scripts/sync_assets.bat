echo Syncing paid assets...
rclone sync gdrive:GY_PaidAssets .\..\Content\ThirdParty\PaidAssets --progress
echo.
echo Done!
pause

