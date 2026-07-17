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
	void ApplyRegenEffects();
	void ApplyActivityPointsUsedEffect();

	/** 리스폰 시 런타임 잔존물(쿨다운/전투상태 등) 전체 제거 후 베이스라인(리젠 GE)만 재적용 */
	void ResetForRespawn();

	void ApplyCombatTag();
	void RemoveCombatTag();
private:
	void ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle);

public:
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<FGYDisableThreshold> DisableThresholds;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaggerRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StunRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> ActivityPointsRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> CombatStateEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> ActivityPointsUsedEffect;

private:
	FActiveGameplayEffectHandle StaggerRegenGEHandle;
	FActiveGameplayEffectHandle StunRegenGEHandle;
	FActiveGameplayEffectHandle CombatStateEffectHandle;
	FActiveGameplayEffectHandle ActivityPointsRegenEffectHandle;
};
