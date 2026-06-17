#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "GYEnemyAbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYEnemyAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	void HandleVitalAccumulation(const FGameplayAttribute& ChangedAttribute, float CurrentValue);
	void NotifyAttributeChanged(const FGameplayAttribute& Attribute);
	void ApplyRegenEffects();

	void ApplyCombatTag();
	void RemoveCombatTag();
private:
	void ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle);
	void RefreshRegenDelay(const FGameplayTag& DelayTag, float Duration);
	void ResetRegenDelays();

public:
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<FGYDisableThreshold> DisableThresholds;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaggerRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StunRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Regen")
	TSubclassOf<UGYRegenDelayEffect> RegenDelayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> CombatStateEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Regen")
	float StaggerRegenDelayDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Regen")
	float StunRegenDelayDuration = 2.f;

private:
	FActiveGameplayEffectHandle StaggerRegenGEHandle;
	FActiveGameplayEffectHandle StunRegenGEHandle;
	FActiveGameplayEffectHandle CombatStateEffectHandle;
};
