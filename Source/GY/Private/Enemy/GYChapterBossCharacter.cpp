#include "Enemy/GYChapterBossCharacter.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Actor/GYWeaponActor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "UI/GYUIMessages.h"

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
	return Found ? *Found : nullptr;
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

void AGYChapterBossCharacter::OnRep_Phase2Weapon()
{
	ApplySecondPhaseWeapon();
}

void AGYChapterBossCharacter::ApplySecondPhaseWeapon()
{
	if (!bPhase2Weapon) return;

	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		InitAnimInstanceAssets(AnimInst, SecondPhaseWeapon.AnimationConfig);
	}
	BuildMontageMap(SecondPhaseWeapon.AnimationConfig);

	if (HasAuthority())
	{
		if (AGYWeaponActor* Old = GetWeaponBySlot(SecondPhaseWeapon.HideWeaponSlot))
		{
			Old->SetActorHiddenInGame(true);
		}
		if (AGYWeaponActor* New = GetWeaponBySlot(SecondPhaseWeapon.ShowWeaponSlot))
		{
			New->SetActorHiddenInGame(false);
		}
		if (SecondPhaseWeapon.NewWeaponTraceSockets.Num() > 0)
		{
			WeaponTraceSockets = SecondPhaseWeapon.NewWeaponTraceSockets;
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
	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
		ActiveSequenceActor = nullptr;
	}
	ActiveSequencePlayer = nullptr;

	FGYCinematicMessage Msg;
	Msg.bIsPlaying = false;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(GYGameplayTags::Message_Cinematic_State, Msg);
}
