#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AITypes.h"
#include "GameplayTagContainer.h"
#include "Navigation/PathFollowingComponent.h"
#include "PhaseMoveTo.generated.h"

UENUM(BlueprintType)
enum class EPhaseMoveTarget : uint8
{
	NamedTagInWorld    UMETA(DisplayName = "Actor With Tag"),
	Explicit           UMETA(DisplayName = "Explicit World Location"),
};

class AAIController;

UCLASS()
class GY_API UPhaseMoveTo : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UPhaseMoveTo();

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
	bool ResolveDestination(FVector& OutLocation) const;

	UFUNCTION()
	void OnAIMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void OnSafetyTimeout();
	void ApplyWalkSpeedOverride(bool bApply);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target")
	EPhaseMoveTarget TargetMode = EPhaseMoveTarget::NamedTagInWorld;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Target",
		meta = (EditCondition = "TargetMode == EPhaseMoveTarget::NamedTagInWorld"))
	FName DestinationActorTag = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Target",
		meta = (EditCondition = "TargetMode == EPhaseMoveTarget::Explicit", MakeEditWidget))
	FVector ExplicitDestination = FVector::ZeroVector;

	/** AcceptanceRadius — 이 거리 이내면 도착으로 인정 */
	UPROPERTY(EditDefaultsOnly, Category = "Move|Move")
	float AcceptanceRadius = 50.f;

	/** 이동 동안 강제 적용할 MaxWalkSpeed (0이면 변경 안 함) */
	UPROPERTY(EditDefaultsOnly, Category = "Move|Move", meta = (ClampMin = "0.0"))
	float WalkSpeedOverride = 0.f;

	/** 경로 못찾거나 길이 막혔을 때 강제 종료까지의 최대 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety", meta = (ClampMin = "0.0"))
	float SafetyTimeout = 10.f;

	/** 시작 시점에 진행 중인 다른 MoveTo가 있으면 중단할지 */
	UPROPERTY(EditDefaultsOnly, Category = "Move|Move")
	bool bStopOngoingMovementOnEnter = true;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Cue")
	FGameplayTag StartCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Cue")
	FGameplayTag ArriveCueTag;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AAIController> CachedAI;

	FAIRequestID CurrentRequestID;
	FTimerHandle SafetyTimerHandle;
	float CachedOriginalWalkSpeed = 0.f;
	bool bWalkSpeedOverridden = false;
};
