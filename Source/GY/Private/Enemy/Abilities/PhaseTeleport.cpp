#include "Enemy/Abilities/PhaseTeleport.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Enemy/Component/BossAggroComponent.h"
#include "TimerManager.h"

UPhaseTeleport::UPhaseTeleport()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPhaseTeleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ResolveDestination(PendingDestination))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bDisableMovementWhileTeleporting)
	{
		LockMovement(true);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (DisappearCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.SourceObject = ASC->GetAvatarActor();
			ASC->ExecuteGameplayCue(DisappearCueTag, Params);
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (DisappearDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(DisappearTimer, this,
			&UPhaseTeleport::HandleDisappearFinished,
			DisappearDuration, false);
	}
	else
	{
		HandleDisappearFinished();
	}
}

void UPhaseTeleport::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPhaseTeleport::HandleDisappearFinished()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	Avatar->SetActorLocation(PendingDestination, false, nullptr, ETeleportType::TeleportPhysics);
	if (!bFaceTargetAfterArrive) return;
	APawn* BossPawn = Cast<APawn>(Avatar);
	AAIController* AI = Cast<AAIController>(BossPawn->GetController());
	if (!AI) return;

	if (UBossAggroComponent* Aggro = AI->FindComponentByClass<UBossAggroComponent>())
	{
		if (AActor* Target = Aggro->GetCurrentTarget())
		{
			FVector Dir = Target->GetActorLocation() - Avatar->GetActorLocation();
			Dir.Z = 0.f;
			if (!Dir.IsNearlyZero())
			{
				Avatar->SetActorRotation(Dir.Rotation());
			}
		}
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (AppearCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.SourceObject = ASC->GetAvatarActor();
			Params.Location = PendingDestination;
			ASC->ExecuteGameplayCue(AppearCueTag, Params);
		}
	}

	if (AppearDuration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(AppearTimer, this, &UPhaseTeleport::HandleAppearFinished,
			AppearDuration, false);
	}
	else
	{
		HandleAppearFinished();
	}
}

void UPhaseTeleport::HandleAppearFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UPhaseTeleport::ResolveDestination(FVector& OutLocation) const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return false;

	switch (TargetMode)
	{
	case EPhaseTeleportTarget::SelfLocation:
		OutLocation = Avatar->GetActorLocation();
		return true;

	case EPhaseTeleportTarget::Explicit:
		OutLocation = ExplicitDestination;
		return true;

	case EPhaseTeleportTarget::NamedTagInWorld:
		{
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

	case EPhaseTeleportTarget::CurrentTarget:
		{
			APawn* BossPawn = Cast<APawn>(Avatar);
			AAIController* AI = Cast<AAIController>(BossPawn->GetController());
			if (!AI) return false;

			UBossAggroComponent* Aggro = AI->FindComponentByClass<UBossAggroComponent>();
			if (!Aggro) return false;

			AActor* Target = Aggro->GetCurrentTarget();
			if (!Target) return false;

			const FVector TargetLoc = Target->GetActorLocation();
			FVector Dir = Avatar->GetActorLocation() - TargetLoc;
			Dir.Z = 0.f;
			if (Dir.IsNearlyZero())
				Dir = FVector::ForwardVector;
			Dir.Normalize();

			OutLocation = TargetLoc + Dir * DistanceFromTarget;
			OutLocation.Z = Avatar->GetActorLocation().Z;
			return true;
		}
	}
	return false;
}

void UPhaseTeleport::LockMovement(bool bLock)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;

	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move) return;

	if (bLock)
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		bMovementLocked = true;
	}
	else
	{
		Move->SetMovementMode(MOVE_Walking);
		bMovementLocked = false;
	}
}
