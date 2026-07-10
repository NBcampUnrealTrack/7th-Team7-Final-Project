#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GYChargeInputLogic.generated.h"

struct FGYChargeMontageSet;
struct FGYCollisionShapeData;

UCLASS()
class GY_API UGYChargeInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

public:
	const FGYCollisionShapeData* GetCurrentCollisionData() const;

	void CancelMaxChargeTimer();

private:
	void ExecuteAttack();

	UFUNCTION()
	void OnMaxChargeFinished();

	UFUNCTION()
	void OnAttackMontageFinished();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	float ChargeStartTime = 0.f;
	bool bCharging = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> MaxChargeTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> MontageEndTask;

	const FGYChargeMontageSet* CachedMontageSet = nullptr;
	const TArray<FGYCollisionShapeData>* CachedCollisions = nullptr;
};
