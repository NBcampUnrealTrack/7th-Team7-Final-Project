#include "Enemy/Abilities/PhaseMoveTo.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UPhaseMoveTo::UPhaseMoveTo()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPhaseMoveTo::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APawn* BossPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AI = BossPawn ? Cast<AAIController>(BossPawn->GetController()) : nullptr;
	if (!AI)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CachedAI = AI;

	FVector Dest;
	if (!ResolveDestination(Dest))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bStopOngoingMovementOnEnter)
	{
		AI->StopMovement();
	}

	ApplyWalkSpeedOverride(true);

	AI->ReceiveMoveCompleted.AddDynamic(this, &UPhaseMoveTo::OnAIMoveCompleted);

	FAIMoveRequest Req;
	Req.SetGoalLocation(Dest);
	Req.SetAcceptanceRadius(AcceptanceRadius);
	Req.SetUsePathfinding(true);
	Req.SetAllowPartialPath(true);

	const FPathFollowingRequestResult Result = AI->MoveTo(Req);
	CurrentRequestID = Result.MoveId;

	if (Result.Code == EPathFollowingRequestResult::Failed)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		OnAIMoveCompleted(CurrentRequestID, EPathFollowingResult::Success);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (StartCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.SourceObject = ASC->GetAvatarActor();
			ASC->ExecuteGameplayCue(StartCueTag, Params);
		}
	}

	if (SafetyTimeout > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(SafetyTimerHandle, this,
			&UPhaseMoveTo::OnSafetyTimeout,
			SafetyTimeout, false);
	}
}

void UPhaseMoveTo::OnAIMoveCompleted(
	FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (CurrentRequestID.IsValid() && RequestID != CurrentRequestID) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ArriveCueTag.IsValid() && Result == EPathFollowingResult::Success)
		{
			FGameplayCueParameters Params;
			Params.SourceObject = ASC->GetAvatarActor();
			ASC->ExecuteGameplayCue(ArriveCueTag, Params);
		}
	}

	const bool bSucceeded = (Result == EPathFollowingResult::Success);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bSucceeded);
}

void UPhaseMoveTo::OnSafetyTimeout()
{
	if (CachedAI.IsValid())
	{
		CachedAI->StopMovement();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPhaseMoveTo::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyTimerHandle);
	}

	if (CachedAI.IsValid())
	{
		CachedAI->ReceiveMoveCompleted.RemoveDynamic(this, &UPhaseMoveTo::OnAIMoveCompleted);
		if (bWasCancelled)
		{
			CachedAI->StopMovement();
		}
	}

	if (bWalkSpeedOverridden)
	{
		ApplyWalkSpeedOverride(false);
	}

	CurrentRequestID = FAIRequestID();
	CachedAI.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPhaseMoveTo::ResolveDestination(FVector& OutLocation) const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return false;

	switch (TargetMode)
	{
	case EPhaseMoveTarget::Explicit:
		OutLocation = ExplicitDestination;
		return true;

	case EPhaseMoveTarget::NamedTagInWorld:
		if (DestinationActorTag.IsNone()) return false;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(DestinationActorTag))
			{
				OutLocation = It->GetActorLocation();
				return true;
			}
		}
		return false;
	}
	return false;
}

void UPhaseMoveTo::ApplyWalkSpeedOverride(bool bApply)
{
	ACharacter* C = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
	if (!Move) return;

	if (bApply)
	{
		if (WalkSpeedOverride > 0.f)
		{
			CachedOriginalWalkSpeed = Move->MaxWalkSpeed;
			Move->MaxWalkSpeed      = WalkSpeedOverride;
			bWalkSpeedOverridden    = true;
		}
	}
	else
	{
		Move->MaxWalkSpeed   = CachedOriginalWalkSpeed;
		bWalkSpeedOverridden = false;
	}
}
