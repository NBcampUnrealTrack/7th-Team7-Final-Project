#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GameplayTagContainer.h"
#include "GYComboInputLogic.generated.h"

class UAnimMontage;
struct FGYCollisionShapeData;

UCLASS()
class GY_API UGYComboInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

	const FGYCollisionShapeData* GetCurrentCollisionData() const;

private:
	void PlayCurrentMontage();
	void AdvanceCombo();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	int32 ComboIndex = 0;
	int32 MaxComboCount = 0;
	int32 ComboIndexAtWindowOpen = 0;
	bool bWindowOpen = false;
	bool bReady = false;
	const TArray<TObjectPtr<UAnimMontage>>* CachedMontages = nullptr;
	const TArray<FGYCollisionShapeData>* CachedCollisions = nullptr;
};
