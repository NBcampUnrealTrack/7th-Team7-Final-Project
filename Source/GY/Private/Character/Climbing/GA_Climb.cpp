#include "Character/Climbing/GA_Climb.h"

#include "NavigationSystem.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Components/BoxComponent.h"
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

void UGA_Climb::OnExitNavSnapFinished()
{
	ExitNavSnapTask = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

}

void UGA_Climb::OnEntryMoveFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CachedMovement.IsValid() || !CurrentLadder.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Character->SetActorRotation(PendingEntryTransform.GetRotation());
	CachedMovement->OnClimbingEnded.AddDynamic(this, &UGA_Climb::OnClimbExit);

	if (bEnteredFromTop && EntryFromTopMontage)
	{

		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(true);
		}

		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, EntryFromTopMontage);
		CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_Climb::OnEntryMontageCompleted);
		CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_Climb::OnEntryMontageInterrupted);
		CurrentMontageTask->OnBlendOut.AddDynamic(this, &UGA_Climb::OnEntryMontageCompleted);
		CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_Climb::OnEntryMontageInterrupted);
		CurrentMontageTask->ReadyForActivation();
	}else
	{
		CachedMovement->StartClimbing(CurrentLadder.Get());
	}
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
	if (CachedMovement.IsValid() && CurrentLadder.IsValid())
	{
		CachedMovement->StartClimbing(CurrentLadder.Get());
	}
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
	if (CachedMovement.IsValid() && CurrentLadder.IsValid())
	{
		CachedMovement->StartClimbing(CurrentLadder.Get());
	}
}

void UGA_Climb::OnExitMontageCompleted()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CurrentLadder.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	//탈출지점
	const FVector Desired = CurrentLadder->GetTopExitNavPoint();

	FVector NavSafeLoc = Desired;
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Character->GetWorld()))
	{
		FNavLocation Proj;
		if (NavSys->ProjectPointToNavigation(Desired, Proj, NavProjectExtent))
		{
			NavSafeLoc = Proj.Location;
		}
	}

	if (CachedMovement.IsValid())
	{
		CachedMovement->SetMovementMode(MOVE_Flying);
	}

	ExitNavSnapTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("LadderExitNavSnap"),
		NavSafeLoc,
		ExitNavSnapDuration,
		false,
		MOVE_Walking,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity,
		FVector::ZeroVector,
		0.f);

	ExitNavSnapTask->OnTimedOut.AddDynamic(this, &UGA_Climb::OnExitNavSnapFinished);
	ExitNavSnapTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Climb::OnExitNavSnapFinished);
	ExitNavSnapTask->ReadyForActivation();
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
	bEnteredFromTop = (TriggerEventData->EventMagnitude > 0.5f) ? true : ShouldEnterFromTop(Character, Ladder);
	const FTransform EntryT = Ladder->GetClimbStartTransform(bEnteredFromTop);
	const FVector CharacterExtent = Character->GetComponentsBoundingBox().GetExtent();

	FVector TargetLoc = EntryT.GetLocation()
		+ FVector(0, 0, CharacterExtent.Z)
		+ (bEnteredFromTop?(-1.f*Ladder->GetActorForwardVector() * CharacterExtent.X):(Ladder->GetActorForwardVector() * CharacterExtent.X));
	if (bEnteredFromTop)
	{
		FVector EntryBoxLocation =CurrentLadder->GetTopEntryBox()->GetComponentLocation();
		TargetLoc = FVector(EntryBoxLocation.X, EntryBoxLocation.Y, EntryBoxLocation.Z+CharacterExtent.Z);
	}else
	{

	}

	PendingEntryTransform = FTransform(EntryT.GetRotation(), TargetLoc);


	SavedMovementMode = Movement->MovementMode;
	bSavedOrientToMovement = Movement->bOrientRotationToMovement;
	bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;

	Movement->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = false;
	Movement->bUseControllerDesiredRotation = false;

	Movement->SetMovementMode(MOVE_Flying);
	EntryMoveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("LadderEntry"),
		TargetLoc,
		EntryInterpDuration,
		false,
		MOVE_Flying,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f
		);

	EntryMoveTask->OnTimedOut.AddDynamic(this, &UGA_Climb::OnEntryMoveFinished);
	EntryMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Climb::OnEntryMoveFinished);

	EntryMoveTask->ReadyForActivation();

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
	if (EntryMoveTask)
	{
		EntryMoveTask->EndTask();
		EntryMoveTask = nullptr;
	}
	if (ExitNavSnapTask)
	{
		ExitNavSnapTask->EndTask();
		ExitNavSnapTask = nullptr;
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
		CachedMovement->SetMovementMode(SavedMovementMode);
		CachedMovement->bOrientRotationToMovement = bSavedOrientToMovement;
		CachedMovement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
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
				Character->SetActorRotation(CurrentLadder->GetClimbFacing());
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
