#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "AttackLogic/Direction/GYDirectionFragment.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_RotateTo.h"
#include "GYDirectionLogic.generated.h"

class AGYCharacter;
class UGYPlayerGameplayAbility;

UCLASS()
class GY_API UGYDirectionLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void BeginRotation();
	TOptional<float> ResolveTargetYaw() const;

	TWeakObjectPtr<AGYCharacter> CachedCharacter;
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;

	EGYDirectionMode CachedDirectionMode = EGYDirectionMode::ByCharacterForward;
	float CachedLerpTime = 0.f;
	bool bCachedCanOverrideLockOn = false;
	bool bSuppressedLockOn = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_RotateTo> ActiveRotateTask;
};
