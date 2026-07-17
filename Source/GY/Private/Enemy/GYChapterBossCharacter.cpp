#include "Enemy/GYChapterBossCharacter.h"

#include "Animation/BlendSpace.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/QuestTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Actor/GYWeaponActor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "UI/GYUIMessages.h"
#include "Character/HitReactionComponent.h"

AGYChapterBossCharacter::AGYChapterBossCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAlwaysRelevant = true;

	if (HitReactionComponent)
	{
		HitReactionComponent->SetKnockbackScale(0.f);
	}
}

void AGYChapterBossCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(GYStateTags::State_Life_Dead);
	}

	DisableGameplay();

	if (HasAuthority())
	{
		HandleDeathAuthority();
	}

	OnEnemyDead.Broadcast(this);
	// 적 처치 퀘스트 - 003 목표
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		{
			FQuestEventMessage QuestMsg;
			QuestMsg.EventTag = GYGameplayTags::Quest_Objective_Kill;
			QuestMsg.TargetId = "Enemy";
			QuestMsg.Count = 1;
			UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);
		}
		//최종관문 부품 - 006 목표
		{
			FQuestEventMessage QuestMsg;
			QuestMsg.EventTag = GYGameplayTags::Quest_Objective_CollectParts;
			QuestMsg.TargetId = "Parts1";
			QuestMsg.Count = 1;
			UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);
		}
	}

	// 사운드
	if (AbilitySystemComponent && DeathCueTag.IsValid())
	{
		AbilitySystemComponent->ExecuteGameplayCue(DeathCueTag);
	}

}

void AGYChapterBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnWeapons();

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UGYVitalAttributeSet::GetCurrentHealthAttribute())
				.AddUObject(this, &AGYChapterBossCharacter::OnPhaseHealthChanged);
		}
	}
}

void AGYChapterBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->OnFinished.RemoveDynamic(this,&AGYChapterBossCharacter::HandleCinematicFinished);
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

AGYWeaponActor* AGYChapterBossCharacter::GetWeaponBySlot(FGameplayTag SlotTag) const
{
	const TObjectPtr<AGYWeaponActor>* Found = EquippedWeapons.Find(SlotTag);
	if (Found && *Found) return *Found;

	TArray<AActor*> Attached;
	GetAttachedActors(Attached);
	for (AActor* Actor : Attached)
	{
		AGYWeaponActor* Weapon = Cast<AGYWeaponActor>(Actor);
		if (Weapon && Weapon->GetWeaponTypeTag() == SlotTag)
		{
			return Weapon;
		}
	}
	return nullptr;
}

void AGYChapterBossCharacter::Multicast_PlayCinematic_Implementation(const FSoftObjectPath& SequencePath)
{
	if (IsRunningDedicatedServer()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ULevelSequence* Sequence = Cast<ULevelSequence>(SequencePath.TryLoad());
	if (!Sequence) return;

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	Settings.bPauseAtEnd = false;

	ALevelSequenceActor* OutActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Settings, OutActor);
	if (!Player) return;

	ActiveSequencePlayer = Player;
	ActiveSequenceActor = OutActor;
	Player->OnFinished.AddDynamic(this, &AGYChapterBossCharacter::HandleCinematicFinished);

	SetCinematicHidden(true);

	FGYCinematicMessage Msg;
	Msg.bIsPlaying = true;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Cinematic_State, Msg);

	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (ActiveSequencePlayer) { ActiveSequencePlayer->Play(); }
		}));
}

void AGYChapterBossCharacter::SwapToSecondPhaseWeapon()
{
	if (!HasAuthority() || bPhase2Weapon) return;

	bPhase2Weapon = true;
	ApplySecondPhaseWeapon();
}

void AGYChapterBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYChapterBossCharacter, bPhase2Weapon);
}

void AGYChapterBossCharacter::SpawnWeapons()
{
	UWorld* World = GetWorld();
	if (!World || !GetMesh()) return;

	for (const FEnemyWeaponSpawn& Def : WeaponsToSpawn)
	{
		if (!Def.WeaponClass) continue;

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGYWeaponActor* Weapon = World->SpawnActor<AGYWeaponActor>(Def.WeaponClass, Params);
		if (!Weapon) continue;

		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, Def.AttachSocket);
		Weapon->SetActorRelativeTransform(Def.RelativeTransform);
		Weapon->SetActorHiddenInGame(Def.bInitiallyHidden);

		EquippedWeapons.Add(Weapon->GetWeaponTypeTag(), Weapon);
	}
}

void AGYChapterBossCharacter::Activate()
{
	Super::Activate();
	if (!bIsActivate) return;

	bPhase2Weapon = false;
	bPhaseTriggered = false;
	ApplyFirstPhaseWeapon();
}

void AGYChapterBossCharacter::OnRep_Phase2Weapon()
{
	if (bPhase2Weapon)
	{
		ApplySecondPhaseWeapon();
	}
	else
	{
		ApplyFirstPhaseWeapon();
	}
}

void AGYChapterBossCharacter::ApplyFirstPhaseWeapon()
{
	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		if (const UEnemyDataAsset* Data = GetEnemyData())
		{
			if (UBlendSpace* DefaultBS = Data->AnimationConfig.LocomotionBlendSpace.LoadSynchronous())
			{
				AnimInst->SetLocomotionBlendSpace(DefaultBS);
			}
		}
	}

	RefreshWeaponVisibility();

	if (HasAuthority())
	{
		if (const AGYChapterBossCharacter* CDO = GetClass()->GetDefaultObject<AGYChapterBossCharacter>())
		{
			WeaponTraceSockets = CDO->WeaponTraceSockets;
		}
	}
}

void AGYChapterBossCharacter::ApplySecondPhaseWeapon()
{
	if (!bPhase2Weapon) return;

	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		if (UBlendSpace* NewBS = SecondPhaseWeapon.LocomotionBlendSpace.LoadSynchronous())
		{
			AnimInst->SetLocomotionBlendSpace(NewBS);
		}
	}

	RefreshWeaponVisibility();

	if (HasAuthority())
	{
		if (SecondPhaseWeapon.NewWeaponTraceSockets.Num() > 0)
		{
			WeaponTraceSockets = SecondPhaseWeapon.NewWeaponTraceSockets;
		}
	}
}

void AGYChapterBossCharacter::SetCinematicHidden(bool bNewHidden)
{
	SetActorHiddenInGame(bNewHidden);

	TArray<AActor*> Attached;
	GetAttachedActors(Attached);
	for (AActor* Actor : Attached)
	{
		if (Cast<AGYWeaponActor>(Actor))
		{
			Actor->SetActorHiddenInGame(bNewHidden);
		}
	}

	if (!bNewHidden)
	{
		RefreshWeaponVisibility();
	}
}

void AGYChapterBossCharacter::RefreshWeaponVisibility()
{
	for (const FEnemyWeaponSpawn& Def : WeaponsToSpawn)
	{
		if (!Def.WeaponClass) continue;

		const AGYWeaponActor* CDO = Def.WeaponClass->GetDefaultObject<AGYWeaponActor>();
		if (!CDO) continue;

		if (AGYWeaponActor* Weapon = GetWeaponBySlot(CDO->GetWeaponTypeTag()))
		{
			Weapon->SetActorHiddenInGame(Def.bInitiallyHidden);
		}
	}

	if (bPhase2Weapon)
	{
		if (AGYWeaponActor* Old = GetWeaponBySlot(SecondPhaseWeapon.HideWeaponSlot))
		{
			Old->SetActorHiddenInGame(true);
		}
		if (AGYWeaponActor* New = GetWeaponBySlot(SecondPhaseWeapon.ShowWeaponSlot))
		{
			New->SetActorHiddenInGame(false);
		}
	}
}

void AGYChapterBossCharacter::OnPhaseHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bPhaseTriggered || bIsDead) return;

	const float Max = AbilitySystemComponent
		? AbilitySystemComponent->GetNumericAttribute(UGYVitalAttributeSet::GetMaxHealthAttribute()) : 0.f;
	if (Max <= 0.f || Data.NewValue > Max * PhaseHealthRatio) return;

	bPhaseTriggered = true;

	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::PhasePending, true);
		}
	}
}

void AGYChapterBossCharacter::HandleCinematicFinished()
{
	if (!ActiveSequencePlayer && !ActiveSequenceActor) return;

	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->OnFinished.RemoveDynamic(this, &AGYChapterBossCharacter::HandleCinematicFinished);
	}

	SetCinematicHidden(false);

	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
		ActiveSequenceActor = nullptr;
	}
	ActiveSequencePlayer = nullptr;

	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			RefreshWeaponVisibility();
		}));

	FGYCinematicMessage Msg;
	Msg.bIsPlaying = false;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Cinematic_State, Msg);
}
