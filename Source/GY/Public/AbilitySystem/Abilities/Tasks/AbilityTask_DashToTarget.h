#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_DashToTarget.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAbilityTask_DashToTarget : public UAbilityTask
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Ability|Task",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UAbilityTask_DashToTarget* CreateDashToTarget(
		UGameplayAbility* OwningAbility,
		AActor* Target,
		TSubclassOf<UGameplayEffect> InMoveSpeedGEClass,
		float DashSpeed,
		float StopDistance,
		float FrontHalfAngleDeg = 45.f,
		bool bUseAcc = true);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnReachedDistance;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnOutOfSight;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnCancelled;

protected:
	TWeakObjectPtr<AActor> TargetActor;
	TWeakObjectPtr<UGameplayAbility> OwningAbilityRef;
	float DashSpeed = 800.f;
	float StopDistanceSq = 0.f;
	float CosFrontHalfAngle = 0.f;
	bool bBroadcasted = false;
	bool bUseAcc = true;
	float SavedAcc = 0.f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UPROPERTY()
	TSubclassOf<UGameplayEffect> MoveSpeedGEClass;
	FActiveGameplayEffectHandle MovementSpeedGEHandle;

};
