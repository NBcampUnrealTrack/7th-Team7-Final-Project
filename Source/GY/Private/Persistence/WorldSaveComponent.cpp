#include "Persistence/WorldSaveComponent.h"

#include "GameModes/GYGameMode.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Quest/QuestSubsystem.h"
#include "Quest/QuestTypes.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

namespace
{
	constexpr float RetryDelaySeconds = 10.f;

	// 월드 시간처럼 트리거 없이 매 틱 변하는 값의 유실 허용 창 상한
	constexpr float PeriodicSaveSeconds = 180.f;

	UWorldSaveComponent* ResolveWorldSaveComponent(UWorld* World)
	{
		if (!IsValid(World)) return nullptr;
		AGameStateBase* GameState = World->GetGameState();
		return IsValid(GameState) ? GameState->FindComponentByClass<UWorldSaveComponent>() : nullptr;
	}

	void WorldSaveCmd(const TArray<FString>& Args, UWorld* World)
	{
		UWorldSaveComponent* Component = ResolveWorldSaveComponent(World);
		if (!IsValid(Component))
		{
			GY_WARN(Network, KDY, "gy.World.Save: WorldSaveComponent not found");
			return;
		}
		Component->RequestSave();
	}

	void WorldLoadCmd(const TArray<FString>& Args, UWorld* World)
	{
		UWorldSaveComponent* Component = ResolveWorldSaveComponent(World);
		if (!IsValid(Component))
		{
			GY_WARN(Network, KDY, "gy.World.Load: WorldSaveComponent not found");
			return;
		}
		Component->LoadAndApply();
	}

	FAutoConsoleCommandWithWorldAndArgs GYWorldSaveCommand(
		TEXT("gy.World.Save"),
		TEXT("Mark world state dirty and save to Supabase"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WorldSaveCmd));

	FAutoConsoleCommandWithWorldAndArgs GYWorldLoadCommand(
		TEXT("gy.World.Load"),
		TEXT("Reload world state from Supabase and apply"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WorldLoadCmd));
}

UWorldSaveComponent::UWorldSaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UWorldSaveComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority()) return;

	// 월드파티션 맵에서만 — 메뉴 맵 등에서 기본 상태가 저장돼 월드 행을 덮는 사고 방지
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetWorldPartition() == nullptr)
	{
		GY_LOG(Network, KDY, "WorldSave disabled (no world partition: %s)", *GetNameSafe(World));
		return;
	}

	bEnabled = true;
	FParse::Value(FCommandLine::Get(), TEXT("WorldId="), WorldId);

	// 퀘스트 변경 = 저장 트리거. 도메인(QuestSubsystem)은 저장을 모르게 유지 — 구독은 여기서만
	UGameInstance* GameInstance = World->GetGameInstance();
	UQuestSubsystem* QuestSubsystem = IsValid(GameInstance) ? GameInstance->GetSubsystem<UQuestSubsystem>() : nullptr;
	if (IsValid(QuestSubsystem))
	{
		QuestSubsystem->OnQuestStarted.AddWeakLambda(this, [this](FGameplayTag) { RequestSave(); });
		QuestSubsystem->OnQuestProgressUpdated.AddWeakLambda(this, [this](FGameplayTag, int32) { RequestSave(); });
		QuestSubsystem->OnQuestCompleted.AddWeakLambda(this, [this](FGameplayTag) { RequestSave(); });
	}

	// 주기 안전망 — 월드 시간처럼 트리거 없는 변화 커버 (dirty 조건 없이 무조건)
	World->GetTimerManager().SetTimer(
		PeriodicTimerHandle,
		this,
		&UWorldSaveComponent::OnPeriodicTimer,
		PeriodicSaveSeconds,
		true
	);

	GY_LOG(Network, KDY, "WorldSave enabled (worldId=%lld) - loading", WorldId);
	LoadAndApply();
}

void UWorldSaveComponent::FlushForShutdown()
{
	if (!bEnabled || !bLoaded || bShutdownFlushed || !GetOwner()->HasAuthority()) return;
	bShutdownFlushed = true;

	if (bSaving)
	{
		// in-flight 중이면 인계 — 프로세스가 그 완료까지 살아있으면 이어서 전송된다
		UGYPersistenceSubsystem* Persistence = ResolvePersistence();
		if (IsValid(Persistence))
		{
			Persistence->HandoffWorldSave(WorldId, ReadWorldLevelDenorm(), BuildSaveData(), CachedSaveVersion);
		}
		return;
	}

	GY_LOG(Network, KDY, "WorldSave shutdown flush (worldId=%lld)", WorldId);
	bDirty = true;
	TrySave();
}

void UWorldSaveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 flush (best-effort) — CharacterSaveComponent 와 동일한 인계 패턴.
	// OnEnginePreExit 에서 이미 발사했으면 중복 전송(무조건 CAS 충돌) 스킵
	if (bEnabled && bLoaded && !bShutdownFlushed && GetOwner()->HasAuthority())
	{
		GY_LOG(Network, KDY, "WorldSave EndPlay flush (saving=%d)", bSaving);
		if (bSaving)
		{
			UGYPersistenceSubsystem* Persistence = ResolvePersistence();
			if (IsValid(Persistence))
			{
				Persistence->HandoffWorldSave(WorldId, ReadWorldLevelDenorm(), BuildSaveData(), CachedSaveVersion);
			}
		}
		else
		{
			bDirty = true;
			TrySave();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
		World->GetTimerManager().ClearTimer(PeriodicTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UWorldSaveComponent::RequestSave()
{
	if (!bEnabled || !GetOwner()->HasAuthority()) return;

	bDirty = true;

	// 다음 틱으로 뭉침 — 같은 프레임의 연쇄 변경(퀘스트 완료→보상→다음 퀘스트 시작)이 끝난 상태를 한 번에
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;
	World->GetTimerManager().SetTimerForNextTick(this, &UWorldSaveComponent::TrySave);
}

void UWorldSaveComponent::TrySave()
{
	// bLoaded 게이팅: 로드(또는 신규 확정) 전 저장은 기본 상태로 월드를 덮는 사고 — 절대 금지
	if (!bEnabled || !bLoaded || bSaving || !bDirty) return;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	bSaving = true;
	bDirty = false;

	Persistence->SaveWorld(
		WorldId,
		ReadWorldLevelDenorm(),
		BuildSaveData(),
		CachedSaveVersion,
		FGYOnSaveComplete::CreateUObject(this, &UWorldSaveComponent::OnSaveDone)
	);
}

void UWorldSaveComponent::OnSaveDone(const FGYSaveResult& Result)
{
	bSaving = false;

	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		CachedSaveVersion = Result.NewVersion;
		if (bDirty)
		{
			TrySave();
		}
		break;

	case EGYPersistResult::Conflict:
		// 다른 서버가 같은 월드를 쓰는 중이거나 버전 어긋남 — 재로드로 재동기화 (로컬은 DB 상태로 덮임)
		GY_WARN(Network, KDY, "World save conflict - reloading to resync (another server hosting world %lld?)", WorldId);
		bLoaded = false;
		LoadAndApply();
		break;

	default:
		bDirty = true;
		ScheduleRetry();
		break;
	}
}

void UWorldSaveComponent::LoadAndApply()
{
	if (!bEnabled || !GetOwner()->HasAuthority()) return;
	if (bLoading) return;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	bLoading = true;
	Persistence->LoadWorld(
		WorldId,
		FGYOnLoadComplete::CreateUObject(this, &UWorldSaveComponent::OnLoadDone)
	);
}

void UWorldSaveComponent::OnLoadDone(const FGYLoadResult& Result)
{
	bLoading = false;

	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		{
			CachedSaveVersion = Result.SaveVersion;
			if (Result.Data.IsValid())
			{
				ApplyLoadedData(Result.Data);
			}
			bDirty = false;
			bLoaded = true;
			GY_LOG(Network, KDY, "World %lld loaded (saveVersion=%d)", WorldId, CachedSaveVersion);
			NotifyWorldStateReady();
			break;
		}

	case EGYPersistResult::NotFound:
		{
			// 정당한 신규 월드 — 행 생성 후 기본 상태로 시작. Failure 와 반드시 구분 (오판 시 데이터 소실)
			GY_LOG(Network, KDY, "World %lld not found - creating", WorldId);
			UGYPersistenceSubsystem* Persistence = ResolvePersistence();
			if (IsValid(Persistence))
			{
				Persistence->CreateWorld(
					WorldId,
					FString::Printf(TEXT("GY World %lld"), WorldId),
					FGYOnSaveComplete::CreateUObject(this, &UWorldSaveComponent::OnCreateDone)
				);
			}
			break;
		}

	default:
		// 네트워크/백엔드 장애 — 저장 봉쇄(bLoaded false) + 재시도. 입장 게이팅도 이 플래그가 막는다
		ScheduleRetry();
		break;
	}
}

void UWorldSaveComponent::OnCreateDone(const FGYSaveResult& Result)
{
	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		if (Result.NewVersion > 0)
		{
			// 행이 이미 있고 저장 이력까지 있음(생성 레이스 등) — 데이터를 다시 로드
			LoadAndApply();
		}
		else
		{
			CachedSaveVersion = 0;
			bLoaded = true;
			GY_LOG(Network, KDY, "World %lld created - starting fresh", WorldId);
			NotifyWorldStateReady();
		}
		break;

	case EGYPersistResult::Conflict:
		// 소프트 삭제된 월드 — 재시도로 낫지 않는 구성 오류. 서버는 쓰되 영속만 비활성
		GY_ERROR(Network, KDY, "World %lld is soft-deleted - world persistence disabled", WorldId);
		bEnabled = false;
		NotifyWorldStateReady();
		break;

	default:
		ScheduleRetry();
		break;
	}
}

void UWorldSaveComponent::ScheduleRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	GY_WARN(Network, KDY, "World persistence retry in %.0fs (worldId=%lld)", RetryDelaySeconds, WorldId);
	World->GetTimerManager().SetTimer(
		RetryTimerHandle,
		this,
		&UWorldSaveComponent::OnRetryTimer,
		RetryDelaySeconds
	);
}

void UWorldSaveComponent::OnRetryTimer()
{
	if (!bLoaded)
	{
		LoadAndApply();
	}
	else
	{
		TrySave();
	}
}

void UWorldSaveComponent::OnPeriodicTimer()
{
	if (!bLoaded) return;
	RequestSave();
}

void UWorldSaveComponent::NotifyWorldStateReady()
{
	// 로드 대기 중 입장한(폰 스폰이 보류된) 컨트롤러들을 GameMode 가 일괄 스폰
	AGYGameMode* GameMode = GetWorld()->GetAuthGameMode<AGYGameMode>();
	if (IsValid(GameMode))
	{
		GameMode->OnWorldStateReady();
	}
}

UGYPersistenceSubsystem* UWorldSaveComponent::ResolvePersistence() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return nullptr;

	UGameInstance* GameInstance = World->GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYPersistenceSubsystem>() : nullptr;
}

int32 UWorldSaveComponent::ReadWorldLevelDenorm() const
{
	const AGYGameState* GameState = Cast<AGYGameState>(GetOwner());
	return IsValid(GameState) ? FMath::RoundToInt(GameState->GetWorldLevel()) : 1;
}

FString UWorldSaveComponent::BuildSaveData() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	const AGYGameState* GameState = Cast<AGYGameState>(GetOwner());
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;

	// quest: 완료/활성 태그(GameState 권위) + 목표 진행도(QuestSubsystem 서버 메모리)
	if (IsValid(GameState))
	{
		const TSharedRef<FJsonObject> Quest = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> Cleared;
		for (const FGameplayTag& Tag : GameState->GetCompletedQuests())
		{
			Cleared.Add(MakeShared<FJsonValueString>(Tag.ToString()));
		}
		Quest->SetArrayField(TEXT("cleared"), Cleared);

		TArray<TSharedPtr<FJsonValue>> Active;
		const UQuestSubsystem* QuestSubsystem = IsValid(GameInstance) ? GameInstance->GetSubsystem<UQuestSubsystem>() : nullptr;
		for (const FGameplayTag& Tag : GameState->GetActiveQuests())
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("tag"), Tag.ToString());

			const FQuestRuntimeData* Runtime = IsValid(QuestSubsystem) ? QuestSubsystem->GetQuestRuntimeData(Tag) : nullptr;
			Entry->SetNumberField(TEXT("state"), Runtime != nullptr ? static_cast<int32>(Runtime->State) : static_cast<int32>(EQuestState::InProgress));
			Entry->SetNumberField(TEXT("progress"), Runtime != nullptr ? Runtime->ObjectiveProgress : 0);
			Active.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Quest->SetArrayField(TEXT("active"), Active);

		Root->SetObjectField(TEXT("quest"), Quest);

		// world: 월드레벨 + 월드 시간
		const TSharedRef<FJsonObject> WorldSection = MakeShared<FJsonObject>();
		WorldSection->SetNumberField(TEXT("level"), GameState->GetWorldLevel());
		WorldSection->SetNumberField(TEXT("time"), GameState->GetCurrentTime());
		Root->SetObjectField(TEXT("world"), WorldSection);
	}

	// reset: 이번 사이클에 비활성화된 액터 + 리셋 시각 (WorldResetSubsystem 의 SaveLoad 접근자)
	const UGYWorldResetSubsystem* ResetSubsystem = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYWorldResetSubsystem>() : nullptr;
	if (ResetSubsystem != nullptr)
	{
		const TSharedRef<FJsonObject> Reset = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> Deactivated;
		for (const FGuid& Guid : ResetSubsystem->GetDeactivatedActors())
		{
			Deactivated.Add(MakeShared<FJsonValueString>(Guid.ToString()));
		}
		Reset->SetArrayField(TEXT("deactivated"), Deactivated);
		Reset->SetStringField(TEXT("reset_remain"), ResetSubsystem->GetWorldResetTime().ToIso8601());

		Root->SetObjectField(TEXT("reset"), Reset);
	}

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

void UWorldSaveComponent::ApplyLoadedData(const TSharedPtr<FJsonObject>& Data)
{
	AGYGameState* GameState = Cast<AGYGameState>(GetOwner());
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;

	const TSharedPtr<FJsonObject>* Quest = nullptr;
	if (IsValid(GameState) && Data->TryGetObjectField(TEXT("quest"), Quest))
	{
		const TArray<TSharedPtr<FJsonValue>>* Cleared = nullptr;
		if ((*Quest)->TryGetArrayField(TEXT("cleared"), Cleared))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Cleared)
			{
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Value->AsString()), false);
				if (Tag.IsValid())
				{
					GameState->AddCompletedQuest(Tag);
				}
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* Active = nullptr;
		if ((*Quest)->TryGetArrayField(TEXT("active"), Active))
		{
			TMap<FGameplayTag, FQuestRuntimeData> Restored;
			for (const TSharedPtr<FJsonValue>& Value : *Active)
			{
				const TSharedPtr<FJsonObject> Entry = Value->AsObject();
				if (!Entry.IsValid()) continue;

				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Entry->GetStringField(TEXT("tag"))), false);
				if (!Tag.IsValid()) continue;

				GameState->AddActiveQuest(Tag);

				FQuestRuntimeData Runtime;
				Runtime.QuestTag = Tag;
				Runtime.State = static_cast<EQuestState>(FMath::Clamp(
					static_cast<int32>(Entry->GetNumberField(TEXT("state"))), 0, static_cast<int32>(EQuestState::Failed)));
				Runtime.ObjectiveProgress = static_cast<int32>(Entry->GetNumberField(TEXT("progress")));
				Restored.Add(Tag, Runtime);
			}

			UQuestSubsystem* QuestSubsystem = IsValid(GameInstance) ? GameInstance->GetSubsystem<UQuestSubsystem>() : nullptr;
			if (IsValid(QuestSubsystem))
			{
				QuestSubsystem->RestoreActiveQuests(Restored);
			}
		}
	}

	const TSharedPtr<FJsonObject>* WorldSection = nullptr;
	if (IsValid(GameState) && Data->TryGetObjectField(TEXT("world"), WorldSection))
	{
		double Level = 1.0;
		if ((*WorldSection)->TryGetNumberField(TEXT("level"), Level))
		{
			GameState->SetWorldLevel(static_cast<float>(Level));
		}
		double Time = 0.0;
		if ((*WorldSection)->TryGetNumberField(TEXT("time"), Time))
		{
			GameState->SetCurrentTime(static_cast<float>(Time));
		}
	}

	const TSharedPtr<FJsonObject>* Reset = nullptr;
	if (Data->TryGetObjectField(TEXT("reset"), Reset))
	{
		UGYWorldResetSubsystem* ResetSubsystem = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYWorldResetSubsystem>() : nullptr;
		if (ResetSubsystem != nullptr)
		{
			const TArray<TSharedPtr<FJsonValue>>* Deactivated = nullptr;
			if ((*Reset)->TryGetArrayField(TEXT("deactivated"), Deactivated))
			{
				TSet<FGuid> Guids;
				for (const TSharedPtr<FJsonValue>& Value : *Deactivated)
				{
					FGuid Guid;
					if (FGuid::Parse(Value->AsString(), Guid))
					{
						Guids.Add(Guid);
					}
				}
				ResetSubsystem->SetDeactivatedActors(Guids);
			}

			FString ResetRemain;
			FDateTime ResetTime;
			if ((*Reset)->TryGetStringField(TEXT("reset_remain"), ResetRemain) && FDateTime::ParseIso8601(*ResetRemain, ResetTime))
			{
				ResetSubsystem->SetWorldResetTime(ResetTime);
			}
		}
	}
}
