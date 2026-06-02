#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "AttackLogic/Direction/GYDirectionFragment.h"
#include "GYDirectionLogic.generated.h"

class AGYCharacter;

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
	void TickRotation();

	TWeakObjectPtr<AGYCharacter> CachedCharacter;
	EGYDirectionMode CachedDirectionMode = EGYDirectionMode::ByCharacterForward;
	float CachedLerpTime = 0.f;
	float StartYaw = 0.f;
	float DeltaYaw = 0.f;
	float LerpDuration = 0.f;
	float RotationStartTime = 0.f;
	FTimerHandle RotationTimer;
};
