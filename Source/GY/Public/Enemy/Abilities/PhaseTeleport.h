#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "PhaseTeleport.generated.h"

UENUM(BlueprintType)
enum class EPhaseTeleportTarget : uint8
{
	SelfLocation       UMETA(DisplayName = "Self (no move)"),
	CurrentTarget      UMETA(DisplayName = "Aggro Target Location"),
	NamedTagInWorld    UMETA(DisplayName = "Actor With Tag"),
	Explicit           UMETA(DisplayName = "Explicit World Location"),
};

UCLASS()
class GY_API UPhaseTeleport : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UPhaseTeleport();
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleDisappearFinished();

	UFUNCTION()
	void HandleAppearFinished();

	bool ResolveDestination(FVector& OutLocation) const;
	void LockMovement(bool bLock);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Target")
	EPhaseTeleportTarget TargetMode = EPhaseTeleportTarget::CurrentTarget;

	UPROPERTY(EditDefaultsOnly, Category ="Teleport|Target",
		meta = (EditCondition = "TargetMode == EPhaseTeleportTarget::NamedTagInWorld"))
	FName DestinationActorTag = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Target",
		meta = (EditCondition = "TargetMode == EPhaseTeleportTarget::Explicit", MakeEditWidget))
	FVector ExplicitDestination = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Target")
	float DistanceFromTarget = 400.f;

	UPROPERTY(EditDefaultsOnly, Category ="Teleport|Timing", meta = (ClampMin = "0.0"))
	float DisappearDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Timing", meta = (ClampMin = "0.0"))
	float AppearDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Cue")
	FGameplayTag DisappearCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Cue")
	FGameplayTag AppearCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Behavior")
	bool bFaceTargetAfterArrive = true;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Behavior")
	bool bDisableMovementWhileTeleporting = true;

private:
	FVector PendingDestination = FVector::ZeroVector;
	FTimerHandle DisappearTimer;
	FTimerHandle AppearTimer;
	bool bMovementLocked = false;
};
