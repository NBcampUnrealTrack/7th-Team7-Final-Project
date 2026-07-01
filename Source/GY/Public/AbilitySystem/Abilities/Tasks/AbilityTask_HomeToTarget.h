#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_HomeToTarget.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAbilityTask_HomeToTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability|Task",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UAbilityTask_HomeToTarget* CreateHomeToTarget(
		UGameplayAbility* OwningAbility,
		AActor* Target,
		//0초주면 무한지속
		float Duration,
		float MaxRotationSpeed = 360.f,
		float InterpSpeed = 8.f,
		bool bUseConstantSpeed = false);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnEnded;

protected:
	TWeakObjectPtr<AActor> TargetActor;

	//0초주면 무한지속
	float Duration = 0.f;
	float Elapsed = 0.f;

	float MaxRotationSpeed = 360.f;
	float InterpSpeed = 8.f;
	bool bUseConstantSpeed = false;

	bool bSavedOrientToMovement = true;
	bool bSavedUseControllerRotationYaw = false;
	bool bSavedUseControllerDesiredRotation = false;
	bool bStateSaved = false;
};
