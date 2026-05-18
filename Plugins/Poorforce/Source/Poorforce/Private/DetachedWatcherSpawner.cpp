#include "DetachedWatcherSpawner.h"

#include "PoorforceLog.h"

#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr int32 PollIntervalSeconds = 2;

	FString BuildWatcherScript()
	{
		return TEXT(R"(param(
    [int]$ParentPid,
    [string]$SentinelPath,
    [string]$LocalPath,
    [string]$RemotePath,
    [string]$UpstashUrl,
    [string]$UpstashToken,
    [string]$LockKey,
    [string]$MyUserId,
    [string]$RcloneExe
)

$ErrorActionPreference = 'SilentlyContinue'

while ($true) {
    Start-Sleep -Seconds )") + FString::FromInt(PollIntervalSeconds) + TEXT(R"(

    if (Test-Path $SentinelPath) {
        Remove-Item -Force $SentinelPath -ErrorAction SilentlyContinue
        exit 0
    }

    $proc = Get-Process -Id $ParentPid -ErrorAction SilentlyContinue
    if ($null -ne $proc) {
        continue
    }

    # Parent died — verify ownership before cleanup
    $headers = @{ Authorization = "Bearer $UpstashToken" }
    $getBody = ConvertTo-Json @("GET", $LockKey) -Compress

    try {
        $getResp = Invoke-RestMethod -Uri $UpstashUrl -Method Post -Headers $headers -ContentType 'application/json' -Body $getBody
    } catch {
        exit 1
    }

    $rawValue = $getResp.result
    if ($null -eq $rawValue) { exit 0 }

    $expectedPrefix = "$MyUserId|"
    if (-not ($rawValue.StartsWith($expectedPrefix))) {
        # Lock now belongs to someone else (force-unlocked). Don't upload.
        exit 0
    }

    if (Test-Path $LocalPath) {
        & $RcloneExe copyto --checksum $LocalPath $RemotePath -v 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { exit 2 }
    }

    $delBody = ConvertTo-Json @("DEL", $LockKey) -Compress
    try {
        Invoke-RestMethod -Uri $UpstashUrl -Method Post -Headers $headers -ContentType 'application/json' -Body $delBody | Out-Null
    } catch {
        exit 3
    }

    exit 0
}
)");
	}
}

FDetachedWatcherSpawner::FDetachedWatcherSpawner(
	FString InRcloneExecutable,
	FString InUpstashUrl,
	FString InUpstashToken,
	FString InUserId)
	: RcloneExecutable(MoveTemp(InRcloneExecutable))
	, UpstashUrl(MoveTemp(InUpstashUrl))
	, UpstashToken(MoveTemp(InUpstashToken))
	, UserId(MoveTemp(InUserId))
{
}

FDetachedWatcherSpawner::~FDetachedWatcherSpawner()
{
	for (TPair<FString, FActiveWatcher>& Pair : Active)
	{
		FFileHelper::SaveStringToFile(FString{}, *Pair.Value.SentinelPath);
	}
	Active.Empty();
}

FString FDetachedWatcherSpawner::GetScriptDir() const
{
	return FPaths::ProjectIntermediateDir() / TEXT("Poorforce");
}

FString FDetachedWatcherSpawner::MakeSafeFilename(const FString& LockKey) const
{
	FString Out = LockKey;
	Out.ReplaceInline(TEXT("/"), TEXT("_"));
	Out.ReplaceInline(TEXT("\\"), TEXT("_"));
	Out.ReplaceInline(TEXT(":"), TEXT("_"));
	Out.ReplaceInline(TEXT(" "), TEXT("_"));
	return Out;
}

bool FDetachedWatcherSpawner::EnsureScriptDirExists() const
{
	const FString Dir = GetScriptDir();
	IFileManager& FM = IFileManager::Get();
	if (FM.DirectoryExists(*Dir)) return true;
	return FM.MakeDirectory(*Dir, /*Tree=*/ true);
}

bool FDetachedWatcherSpawner::SpawnWatcher(
	const FString& LockKey,
	const FString& LocalFilePath,
	const FString& RemoteFilePath)
{
	if (Active.Contains(LockKey))
	{
		UE_LOG(LogPoorforce, Verbose, TEXT("Watcher already active for %s, skipping spawn"), *LockKey);
		return true;
	}

	if (!EnsureScriptDirExists())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Could not create watcher script directory: %s"), *GetScriptDir());
		return false;
	}

	const FString SafeName     = MakeSafeFilename(LockKey);
	const FString ScriptPath   = GetScriptDir() / FString::Printf(TEXT("watcher_%s.ps1"), *SafeName);
	const FString SentinelPath = GetScriptDir() / FString::Printf(TEXT("watcher_%s.sentinel"), *SafeName);

	if (!FFileHelper::SaveStringToFile(BuildWatcherScript(), *ScriptPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Could not write watcher script: %s"), *ScriptPath);
		return false;
	}

	const uint32 ParentPid = FPlatformProcess::GetCurrentProcessId();

	auto QuoteArg = [](const FString& In) -> FString
	{
		return FString::Printf(TEXT("\"%s\""), *In.Replace(TEXT("\""), TEXT("`\"")));
	};

	const FString Args = FString::Printf(
		TEXT("-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File %s ")
		TEXT("-ParentPid %u -SentinelPath %s -LocalPath %s -RemotePath %s ")
		TEXT("-UpstashUrl %s -UpstashToken %s -LockKey %s -MyUserId %s -RcloneExe %s"),
		*QuoteArg(ScriptPath),
		ParentPid,
		*QuoteArg(SentinelPath),
		*QuoteArg(LocalFilePath),
		*QuoteArg(RemoteFilePath),
		*QuoteArg(UpstashUrl),
		*QuoteArg(UpstashToken),
		*QuoteArg(LockKey),
		*QuoteArg(UserId),
		*QuoteArg(RcloneExecutable));

	FProcHandle Handle = FPlatformProcess::CreateProc(
		TEXT("powershell.exe"),
		*Args,
		/*bLaunchDetached=*/ true,
		/*bLaunchHidden=*/ true,
		/*bLaunchReallyHidden=*/ true,
		/*OutProcessID=*/ nullptr,
		/*PriorityModifier=*/ 0,
		/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
		/*PipeWriteChild=*/ nullptr,
		/*PipeReadChild=*/ nullptr);

	if (!Handle.IsValid())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Could not spawn watcher process for %s"), *LockKey);
		IFileManager::Get().Delete(*ScriptPath, /*RequireExists=*/ false, /*EvenReadOnly=*/ true, /*Quiet=*/ true);
		return false;
	}

	FPlatformProcess::CloseProc(Handle);

	Active.Add(LockKey, FActiveWatcher{ ScriptPath, SentinelPath });

	UE_LOG(LogPoorforce, Log, TEXT("Watcher spawned for %s (parent pid=%u)"), *LockKey, ParentPid);
	return true;
}

void FDetachedWatcherSpawner::SignalWatcherExit(const FString& LockKey)
{
	FActiveWatcher* Found = Active.Find(LockKey);
	if (Found == nullptr) return;

	if (!FFileHelper::SaveStringToFile(FString{}, *Found->SentinelPath))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Could not write sentinel for %s; watcher may run cleanup"), *LockKey);
	}
	else
	{
		UE_LOG(LogPoorforce, Log, TEXT("Watcher exit signalled for %s"), *LockKey);
	}

	Active.Remove(LockKey);
}

bool FDetachedWatcherSpawner::IsWatcherActive(const FString& LockKey) const
{
	return Active.Contains(LockKey);
}

void FDetachedWatcherSpawner::CleanupStaleArtifacts()
{
	const FString Dir = GetScriptDir();
	IFileManager& FM = IFileManager::Get();
	if (!FM.DirectoryExists(*Dir)) return;

	TArray<FString> StaleFiles;
	FM.FindFiles(StaleFiles, *(Dir / TEXT("watcher_*.ps1")), /*Files=*/ true, /*Directories=*/ false);
	FM.FindFiles(StaleFiles, *(Dir / TEXT("watcher_*.sentinel")), /*Files=*/ true, /*Directories=*/ false);

	int32 Deleted = 0;
	for (const FString& Name : StaleFiles)
	{
		const FString Full = Dir / Name;
		if (FM.Delete(*Full, /*RequireExists=*/ false, /*EvenReadOnly=*/ true, /*Quiet=*/ true))
		{
			++Deleted;
		}
	}

	if (Deleted > 0)
	{
		UE_LOG(LogPoorforce, Log, TEXT("Cleaned up %d stale watcher artifact(s) in %s"), Deleted, *Dir);
	}
}
