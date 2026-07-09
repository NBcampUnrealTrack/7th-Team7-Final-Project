#include "WorldGimmick/GYEndingInteractActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Engine/Engine.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Components/BoxComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/QuestTags.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "UI/GYUIMessages.h"

AGYEndingInteractActor::AGYEndingInteractActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetCollisionProfileName(GYCollisionProfile::Interactable);
	InteractionBox->SetupAttachment(Root);

	InteractTag = GYGameplayTags::Interaction_Ending_Start;
	InteractionText = NSLOCTEXT("CinematicGate", "Default", "상호작용");
}

void AGYEndingInteractActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGYEndingInteractActor, Mode);
	DOREPLIFETIME(AGYEndingInteractActor, bConsumed);
}

void AGYEndingInteractActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PostCinematicTimerHandle);

	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->OnFinished.RemoveDynamic(this, &AGYEndingInteractActor::HandleCinematicFinished);
		ActiveSequencePlayer->Stop();
		ActiveSequencePlayer = nullptr;
	}

	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
		ActiveSequenceActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AGYEndingInteractActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	if (!Interactor) return;
	if (bConsumed) return;

	if (RequiredStateTag.IsValid()) // 보스 잡기 전엔 상호작용x 하도록
	{
		const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Interactor->GetPlayerState());
		UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !ASC->HasMatchingGameplayTag(RequiredStateTag)) return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.OptionTag = InteractTag;
	Option.SourceObject = const_cast<AGYEndingInteractActor*>(this);
	OutOptions.Add(Option);
}

void AGYEndingInteractActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (OptionTag != InteractTag) return;
	if (!HasAuthority()) return;
	if (bConsumed) return;

	APlayerState* PS = Interactor ? Interactor->GetPlayerState() : nullptr;
	if (!PS) return;

	// 튕긴 플레이어 정리
	InteractedPlayers.RemoveAll([](const TWeakObjectPtr<APlayerState>& W)
	{
		return !W.IsValid();
	});

	RequiredCount = GetCurrentPlayerCount();
	const TWeakObjectPtr<APlayerState> WeakPS(PS);

	// 이미 누른 사람이 다시 누르면 취소 아니면 추가
	bool bAdded;
	if (InteractedPlayers.Contains(WeakPS))
	{
		InteractedPlayers.Remove(WeakPS);
		bAdded = false;
	}
	else
	{
		InteractedPlayers.Add(WeakPS);
		bAdded = true;
	}

	// 시계탑 찾아가기 퀘스트 - 004 목표
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		FQuestEventMessage QuestMsg;
		QuestMsg.EventTag = GYGameplayTags::Quest_Objective_Interact;
		QuestMsg.TargetId = "Intro";
		QuestMsg.Count = 1;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);
	}

	const int32 Current = InteractedPlayers.Num();
	if (bAdded && Current >= RequiredCount)
	{
		bConsumed = true;
		Multicast_PlayCinematic(Cinematic.ToSoftObjectPath()); // 시네마틱 전체 작동
		GetWorldTimerManager().SetTimer(PostCinematicTimerHandle, this,
			&AGYEndingInteractActor::OnPostCinematicTimerExpired, CinematicDuration, false);
	}
	else
	{
		Multicast_NotifyWaiting(PS, Current, RequiredCount, bAdded);
	}
}

int32 AGYEndingInteractActor::GetCurrentPlayerCount() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS) return 1;

	int32 Count = 0;
	for (const APlayerState* PS : GS->PlayerArray)
	{
		if (PS && !PS->IsInactive() && !PS->IsOnlyASpectator())
		{
			++Count;
		}
	}
	return FMath::Max(Count, 1);
}

void AGYEndingInteractActor::Multicast_NotifyWaiting_Implementation(
	APlayerState* ChangedPlayer, int32 Current, int32 Required, bool bAdded)
{
	if (IsRunningDedicatedServer()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FGYInteractionWaitingMessage Msg;
	Msg.CurrentCount = Current;
	Msg.RequiredCount = Required;
	Msg.ChangedPlayer = ChangedPlayer;
	Msg.bAdded = bAdded;

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Ending_WaitingForPlayers, Msg);
}

void AGYEndingInteractActor::Multicast_PlayCinematic_Implementation(const FSoftObjectPath& SequencePath)
{
	if (IsRunningDedicatedServer()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FGYEndingStartedMessage Started;
	Started.Cinematic = TSoftObjectPtr<ULevelSequence>(SequencePath);
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Ending_Started, Started);

	ULevelSequence* Sequence = Cast<ULevelSequence>(SequencePath.TryLoad());
	if (!Sequence)
	{
		BroadcastCinematicFinishedLocal();
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	Settings.bPauseAtEnd = true;

	ALevelSequenceActor* OutActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
	   World, Sequence, Settings, OutActor);

	if (!Player)
	{
		BroadcastCinematicFinishedLocal();
		return;
	}
	ActiveSequencePlayer = Player;
	ActiveSequenceActor = OutActor;

	if (Mode == EGYCinematicGateMode::Intro)
	{
		// 실제 보스 액터를 찾아 Binding Tag로 런타임에 주입
		TArray<AActor*> FoundBosses;
		UGameplayStatics::GetAllActorsOfClass(World, AGYBossCharacterBase::StaticClass(), FoundBosses);
		if (FoundBosses.Num() > 0)
		{
			OutActor->AddBindingByTag(BossBindingTag, FoundBosses[0], /*bAllowBindingsFromAsset=*/ false);
		}
		else
		{
			GY_WARN(Network, CYS, "[%s] No boss actor found to bind for tag '%s'",
				*GetName(), *BossBindingTag.ToString());
		}
	}

	Player->OnFinished.AddDynamic(this, &AGYEndingInteractActor::HandleCinematicFinished);

	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* LocalPawn = LocalPC->GetPawn())
		{
			LocalPawn->SetActorHiddenInGame(true);
		}
	}

	// UI 꺼줘
	FGYCinematicMessage Msg;
	Msg.bIsPlaying = true;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Cinematic_State, Msg);

	Player->Play();
}

void AGYEndingInteractActor::HandleCinematicFinished()
{
	BroadcastCinematicFinishedLocal();

	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
		ActiveSequenceActor = nullptr;
	}
	ActiveSequencePlayer = nullptr;

	// UI 켜줘
	FGYCinematicMessage Msg;
	Msg.bIsPlaying = false;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Cinematic_State, Msg);
}

void AGYEndingInteractActor::OnPostCinematicTimerExpired()
{
	// 시네마틱 타이머 종료 후 로직
	if (!HasAuthority()) return;

	if (PostCinematicSpawnPoint)
	{
		AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
		if (GS)
		{
			const FVector DestLoc = PostCinematicSpawnPoint->GetActorLocation();
			const FRotator DestRot = PostCinematicSpawnPoint->GetActorRotation();

			for (APlayerState* PS : GS->PlayerArray)
			{
				if (PS && PS->GetPawn())
				{
					// 플레이어 이동
					PS->GetPawn()->TeleportTo(DestLoc, DestRot);
				}
			}
		}
	}

	if (BlockingActor)
	{
		// 보스전 블로킹 제거
		BlockingActor->Destroy();
	}

	Multicast_ShowAllPawns();
}

void AGYEndingInteractActor::Multicast_ShowAllPawns_Implementation()
{
	if (IsRunningDedicatedServer()) return;

	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* LocalPawn = LocalPC->GetPawn())
		{
			LocalPawn->SetActorHiddenInGame(false);
		}
	}
}

void AGYEndingInteractActor::BroadcastCinematicFinishedLocal()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FGYEndingCinematicFinishedMessage Msg;
	Msg.bShowCredits = (Mode == EGYCinematicGateMode::Ending);

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(
		GYGameplayTags::Message_Ending_CinematicFinished, Msg);
}

void AGYEndingInteractActor::RevealAtDesignatedLocation()
{
	if (!HasAuthority() || bRevealed || !RevealTargetPoint) return;

	bRevealed = true;
	SetActorLocation(RevealTargetPoint->GetActorLocation());
	SetActorRotation(RevealTargetPoint->GetActorRotation());
}
