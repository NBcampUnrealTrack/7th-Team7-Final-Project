#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "GYGameplayAbility.generated.h"

UENUM(BlueprintType)
enum class EGYAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive,
	OnSpawn
};

UCLASS(Abstract, HideCategories = Input)
class GY_API UGYGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	EGYAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Ability Activation")
	EGYAbilityActivationPolicy ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
};
