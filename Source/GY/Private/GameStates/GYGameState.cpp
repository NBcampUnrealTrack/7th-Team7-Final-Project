#include "GameStates/GYGameState.h"

#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Experience/GYExperienceManagerComponent.h"
#include "Persistence/WorldSaveComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Core/Sound/GYSoundManager.h"
#include "Logging/GYLogManager.h"

AGYGameState::AGYGameState()
{
	ExperienceManagerComponent = CreateDefaultSubobject<UGYExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
	WorldSaveComponent = CreateDefaultSubobject<UWorldSaveComponent>(TEXT("WorldSaveComponent"));
}

UGYExperienceManagerComponent* AGYGameState::GetExperienceManagerComponent() const
{
	return ExperienceManagerComponent;
}

UWorldSaveComponent* AGYGameState::GetWorldSaveComponent() const
{
	return WorldSaveComponent;
}

void AGYGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYGameState, CurrentTime);
	DOREPLIFETIME(AGYGameState, TimeScale);
	DOREPLIFETIME(AGYGameState, ClearedQuests);
	DOREPLIFETIME(AGYGameState, ActiveQuestTags);
	DOREPLIFETIME(AGYGameState, bGameBGMStarted);
}

void AGYGameState::SetCurrentTime(float InCurrentTime)
{
	CurrentTime = InCurrentTime;
	BroadcastTimeChanged();
}

void AGYGameState::OnRep_CurrentTime()
{
	BroadcastTimeChanged();
}

void AGYGameState::OnRep_TimeScale()
{
}

bool AGYGameState::IsQuestComplete(const FGameplayTag QuestTag) const
{
	return ClearedQuests.Contains(QuestTag);
}

void AGYGameState::AddCompletedQuest(FGameplayTag QuestTag)
{
	if (!HasAuthority()) return;
	ClearedQuests.AddUnique(QuestTag);
}

void AGYGameState::AddActiveQuest(FGameplayTag QuestTag)
{
	if (!HasAuthority()) return;
	ActiveQuestTags.AddUnique(QuestTag);
}

void AGYGameState::RemoveActiveQuest(FGameplayTag QuestTag)
{
	if (!HasAuthority()) return;
	ActiveQuestTags.Remove(QuestTag);
}

void AGYGameState::OnRep_ActiveQuests()
{
	for (const FGameplayTag& Tag : ActiveQuestTags)
	{
		if (!PreviousActiveQuestTags.Contains(Tag))
		{
			FGYQuestProgressMessage Msg;
			Msg.QuestId = Tag;
			UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Quest_Started, Msg);
		}
	}
	PreviousActiveQuestTags = ActiveQuestTags;
}

void AGYGameState::OnRep_ClearedQuests()
{
	for (int32 i = PreviousClearedCount; i < ClearedQuests.Num(); i++)
	{
		FGYQuestProgressMessage Msg;
		Msg.QuestId = ClearedQuests[i];
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Quest_Completed, Msg);
	}
	PreviousClearedCount = ClearedQuests.Num();
}

void AGYGameState::PlayGameBGMLocal(FGameplayTag BGMTag) const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (UGYSoundManager* SoundMgr = UGYSoundManager::Get(this))
	{
		SoundMgr->PlayBGM(BGMTag, 0.5f);
	}
}

void AGYGameState::StartGameBGM()
{
	if (!HasAuthority() || bGameBGMStarted) return;

	bGameBGMStarted = true;
	OnRep_GameBGMStarted(); // 서버 자신(리슨서버 호스트)은 리플리케이션을 안 타므로 직접 호출
}

void AGYGameState::OnRep_GameBGMStarted()
{
	PlayGameBGMLocal(GYGameplayTags::Sound_BGM_Lobby); // 임시, 안전장치, 로비 로직 나오면 다시 수정할듯.
}

void AGYGameState::BeginPlay()
{
	Super::BeginPlay();
	if (const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>())
	{
		CurrentTime = ActorGuidDataSettings->StartOfDayHour * 60.f * 60.f;
		TimeScale = ((ActorGuidDataSettings->EndOfDayHour - ActorGuidDataSettings->StartOfDayHour) * 60.f) / ActorGuidDataSettings->RealMinutesPerGameDay;
	}
}

void AGYGameState::BroadcastTimeChanged()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 세계 시간 체크 및 GMS 브로드캐스트
	const int32 TotalSeconds = FMath::FloorToInt(CurrentTime);
	const int32 MinuteOfDay = TotalSeconds / 60;

	if (MinuteOfDay == LastBroadcastedMinute) return; // 같은 Minute면 스킵
	LastBroadcastedMinute = MinuteOfDay;

	FGYWorldTimeMessage TimePayload;
	TimePayload.Hours = (TotalSeconds / 3600) % 24;
	TimePayload.Minutes = MinuteOfDay % 60;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_World_TimeChanged, TimePayload);
}

void AGYGameState::NotifyWorldResetAll(float DurationOverride)
{
	if (!HasAuthority()) return;
	Multicast_PlayWorldResetSequence(DurationOverride);

	// 월드 리셋 이펙트 - 월드 파티션 없으면 실행 안됨(위에서 조건 짤림)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = Cast<APlayerController>(*It);
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
		UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (ASC)
		{
			ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_World_Reset);
		}
	}
}

void AGYGameState::Multicast_PlayWorldResetSequence_Implementation(float DurationOverride)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (IsRunningDedicatedServer()) return;

	FGYWorldResetMessage Payload;
	Payload.DurationOverride = DurationOverride;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(
		GYGameplayTags::Message_World_Reset, Payload);
}
