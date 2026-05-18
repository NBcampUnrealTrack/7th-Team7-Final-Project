#include "PoorforceConfig.h"

#include "PoorforceLog.h"
#include "PoorforceTimeFormat.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace PoorforceConfigLoader
{
	static constexpr const TCHAR* ConfigFileName = TEXT("PoorforceConfig.json");

	static bool ParsePathMode(const FString& InRaw, EPoorforcePathMode& OutMode)
	{
		if (InRaw.Equals(TEXT("LockOnly"), ESearchCase::IgnoreCase))
		{
			OutMode = EPoorforcePathMode::LockOnly;
			return true;
		}

		if (InRaw.Equals(TEXT("LockAndSync"), ESearchCase::IgnoreCase))
		{
			OutMode = EPoorforcePathMode::LockAndSync;
			return true;
		}

		return false;
	}

	FString GetExpectedConfigPath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ConfigFileName);
	}

	bool LoadFromProjectRoot(FPoorforceConfig& OutConfig)
	{
		OutConfig = FPoorforceConfig{};

		const FString ConfigPath = GetExpectedConfigPath();

		if (!FPaths::FileExists(ConfigPath))
		{
			UE_LOG(LogPoorforce, Warning, TEXT("Config file not found: %s"), *ConfigPath);
			return false;
		}

		FString RawJson;
		if (!FFileHelper::LoadFileToString(RawJson, *ConfigPath))
		{
			UE_LOG(LogPoorforce, Error, TEXT("Failed to read config file: %s"), *ConfigPath);
			return false;
		}

		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawJson);

		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogPoorforce, Error, TEXT("Failed to parse config JSON: %s"), *ConfigPath);
			return false;
		}

		Root->TryGetStringField(TEXT("UpstashUrl"), OutConfig.UpstashUrl);
		Root->TryGetStringField(TEXT("UpstashToken"), OutConfig.UpstashToken);
		Root->TryGetStringField(TEXT("LockKeyNamespace"), OutConfig.LockKeyNamespace);
		Root->TryGetStringField(TEXT("DiscordWebhookUrl"), OutConfig.DiscordWebhookUrl);

		FString RcloneExe;
		if (Root->TryGetStringField(TEXT("RcloneExecutable"), RcloneExe) && !RcloneExe.IsEmpty())
		{
			OutConfig.RcloneExecutable = RcloneExe;
		}

		auto ReadTtl = [&Root](const TCHAR* FieldName, int32& OutValue)
		{
			FString Raw;
			if (!Root->TryGetStringField(FieldName, Raw) || Raw.IsEmpty()) return;

			int32 Seconds = 0;
			if (PoorforceTimeFormat::ParseDuration(Raw, Seconds))
			{
				OutValue = Seconds;
			}
			else
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Invalid duration for %s: '%s' (using default %d sec)"),
					FieldName, *Raw, OutValue);
			}
		};

		ReadTtl(TEXT("LockOnlyTtl"), OutConfig.LockOnlyTtlSeconds);
		ReadTtl(TEXT("LockAndSyncTtl"), OutConfig.LockAndSyncTtlSeconds);

		const TArray<TSharedPtr<FJsonValue>>* ManagedPathsJson = nullptr;
		if (Root->TryGetArrayField(TEXT("ManagedPaths"), ManagedPathsJson) && ManagedPathsJson != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& Entry : *ManagedPathsJson)
			{
				const TSharedPtr<FJsonObject>* EntryObj = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetObject(EntryObj) || EntryObj == nullptr)
				{
					UE_LOG(LogPoorforce, Warning, TEXT("ManagedPaths entry is not an object, skipping"));
					continue;
				}

				FPoorforceManagedPath Managed;
				if (!(*EntryObj)->TryGetStringField(TEXT("ContentPath"), Managed.ContentPath) || Managed.ContentPath.IsEmpty())
				{
					UE_LOG(LogPoorforce, Warning, TEXT("ManagedPaths entry missing ContentPath, skipping"));
					continue;
				}

				if (!Managed.ContentPath.EndsWith(TEXT("/")))
				{
					Managed.ContentPath += TEXT("/");
				}

				FString ModeRaw;
				if (!(*EntryObj)->TryGetStringField(TEXT("Mode"), ModeRaw) || !ParsePathMode(ModeRaw, Managed.Mode))
				{
					UE_LOG(LogPoorforce, Warning, TEXT("ManagedPaths entry has invalid Mode for '%s', defaulting to LockOnly"), *Managed.ContentPath);
					Managed.Mode = EPoorforcePathMode::LockOnly;
				}

				(*EntryObj)->TryGetStringField(TEXT("RcloneRemote"), Managed.RcloneRemote);

				if (Managed.Mode == EPoorforcePathMode::LockAndSync && Managed.RcloneRemote.IsEmpty())
				{
					UE_LOG(LogPoorforce, Warning, TEXT("ManagedPath '%s' is LockAndSync but RcloneRemote is empty"), *Managed.ContentPath);
				}

				OutConfig.ManagedPaths.Add(MoveTemp(Managed));
			}
		}

		if (!OutConfig.IsValid())
		{
			UE_LOG(LogPoorforce, Error, TEXT("Config loaded but missing required fields (UpstashUrl/UpstashToken)"));
			return false;
		}

		UE_LOG(LogPoorforce, Log, TEXT("Config loaded: %d managed path(s), namespace='%s'"),
			OutConfig.ManagedPaths.Num(), *OutConfig.LockKeyNamespace);

		return true;
	}
}
