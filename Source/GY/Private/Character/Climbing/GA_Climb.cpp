#include "Character/Climbing/GA_Climb.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WorldGimmick/Ladder.h"


UGA_Climb::UGA_Climb(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationOwnedTags.AddTag(GYStateTags::State_Climbing);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = GYGameplayTags::Event_Ladder_ClimbRequest;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

bool UGA_Climb::ShouldEnterFromTop(const ACharacter* Character, const ALadder* Ladder) const
{
	if (!Character || !Ladder) return false;

	const float CharZ = Character->GetActorLocation().Z;
	const float LadderTopZ = Ladder->GetActorLocation().Z + Ladder->GetClimbDistance();

	return CharZ > (LadderTopZ - TopEntryThreshold);
}

void UGA_Climb::OnEntryMontageCompleted()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(false);
		}
	}
	CachedMovement->SetMovementMode(MOVE_Custom, CMOVE_Climbing);
}

void UGA_Climb::OnEntryMontageInterrupted()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(false);
		}
	}
	CachedMovement->SetMovementMode(MOVE_Custom, CMOVE_Climbing);
}

void UGA_Climb::OnExitMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Climb::OnExitMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Climb::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ALadder* Ladder = const_cast<ALadder*>(Cast<ALadder>(TriggerEventData->OptionalObject));
	if (!Ladder || !Ladder->CanClimb())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UGYCharacterMovementComponent* Movement =
		Character ? Cast<UGYCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;
	if (!Character || !Movement)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentLadder = Ladder;
	CachedMovement = Movement;
	const bool bFromTop = (TriggerEventData->EventMagnitude > 0.5f) ? true : ShouldEnterFromTop(Character, Ladder);
	const FTransform EntryT = Ladder->GetClimbStartTransform(bFromTop);


	SavedMovementMode = Movement->MovementMode;
	Movement->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = false;

	Movement->OnClimbingEnded.AddDynamic(this, &UGA_Climb::OnClimbExit);
	Movement->StartClimbing(Ladder);

	if (!bFromTop)
	{
		if (ActorInfo->IsNetAuthority())
		{
			const FVector CharacterExtent = Character->GetComponentsBoundingBox().GetExtent();
			Character->SetActorLocationAndRotation(
				EntryT.GetLocation() + FVector(0, 0, CharacterExtent.Z) + Ladder->GetActorForwardVector() *
				CharacterExtent.X,
				EntryT.GetRotation(),
				false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	}

	if (bFromTop && EntryFromTopMontage)
	{
		Character->SetActorRotation(EntryT.GetRotation());

		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(true);
		}

		if (CachedMovement.IsValid())
		{
			CachedMovement->SetMovementMode(MOVE_Flying);
		}

		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, EntryFromTopMontage);
		CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_Climb::OnEntryMontageCompleted);
		CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_Climb::OnEntryMontageInterrupted);
		CurrentMontageTask->OnBlendOut.AddDynamic(this, &UGA_Climb::OnEntryMontageCompleted);
		CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_Climb::OnEntryMontageInterrupted);
		CurrentMontageTask->ReadyForActivation();
	}
}

void UGA_Climb::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(false);
		}
		Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	}

	if (CachedMovement.IsValid())
	{
		CachedMovement->OnClimbingEnded.RemoveDynamic(this, &UGA_Climb::OnClimbExit);
		CachedMovement->StopClimbing();
		CachedMovement->SetMovementMode(MOVE_Walking);
		CachedMovement->bOrientRotationToMovement = bSavedOrientToMovement;
	}

	CurrentLadder.Reset();
	CachedMovement.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Climb::OnClimbExit(ELadderExitReason Reason)
{
	if (Reason == ELadderExitReason::Top && ExitToTopMontage)
	{
		ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

		if (Character)
		{
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				PC->SetIgnoreMoveInput(true);
			}
			if (CurrentLadder.IsValid())
			{
				Character->SetActorRotation(-1 * CurrentLadder->GetActorRotation());
			}
		}

		if (CachedMovement.IsValid())
		{
			CachedMovement->SetMovementMode(MOVE_Flying);
		}

		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ExitToTopMontage);
		CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_Climb::OnExitMontageCompleted);
		CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_Climb::OnExitMontageInterrupted);
		CurrentMontageTask->OnBlendOut.AddDynamic(this, &UGA_Climb::OnExitMontageCompleted);
		CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_Climb::OnExitMontageInterrupted);
		CurrentMontageTask->ReadyForActivation();
		return;
	}

	//
	// if (Reason == ELadderExitReason::Top && Character && CurrentLadder.IsValid()&& CurrentActorInfo->IsNetAuthority())
	// {
	// 	const FVector ExitLoc = CurrentLadder->GetActorLocation()
	// 		+ CurrentLadder->GetClimbAxis() * CurrentLadder->GetClimbDistance()
	// 		- CurrentLadder->GetActorForwardVector() * 50.0f
	// 		+ FVector(0,0,Character->GetComponentsBoundingBox().GetExtent().Z);
	// 	Character->SetActorLocation(ExitLoc, false, nullptr, ETeleportType::TeleportPhysics);
	// }

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
