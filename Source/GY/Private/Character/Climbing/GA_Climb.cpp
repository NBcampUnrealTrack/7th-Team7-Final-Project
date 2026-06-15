#include "Character/Climbing/GA_Climb.h"

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
	const bool bFromTop = ShouldEnterFromTop(Character, Ladder);

	const FTransform EntryT = Ladder->GetClimbStartTransform(bFromTop);

	if (ActorInfo->IsNetAuthority())
	{
		const FVector CharacterExtent = Character->GetComponentsBoundingBox().GetExtent();
		Character->SetActorLocationAndRotation(
			EntryT.GetLocation() + FVector(0,0,CharacterExtent.Z) + Ladder->GetActorForwardVector()*CharacterExtent.X,
			EntryT.GetRotation(),
			false, nullptr,
			ETeleportType::TeleportPhysics);
	}


	SavedMovementMode = Movement->MovementMode;
	Movement->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = false;

	Movement->OnClimbingEnded.AddDynamic(this, &UGA_Climb::OnClimbExit);
	Movement->StartClimbing(Ladder);
}

void UGA_Climb::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	if (CachedMovement.IsValid())
	{
		CachedMovement->OnClimbingEnded.RemoveDynamic(this, &UGA_Climb::OnClimbExit);
		CachedMovement->StopClimbing();
		CachedMovement->bOrientRotationToMovement = bSavedOrientToMovement;

		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		}
	}

	CurrentLadder.Reset();
	CachedMovement.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Climb::OnClimbExit(ELadderExitReason Reason)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (Reason == ELadderExitReason::Top && Character && CurrentLadder.IsValid()&& CurrentActorInfo->IsNetAuthority())
	{
		const FVector ExitLoc = CurrentLadder->GetActorLocation()
			+ CurrentLadder->GetClimbAxis() * CurrentLadder->GetClimbDistance()
			- CurrentLadder->GetActorForwardVector() * 50.0f + FVector(0,0,Character->GetComponentsBoundingBox().GetExtent().Z);
		Character->SetActorLocation(ExitLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

}
