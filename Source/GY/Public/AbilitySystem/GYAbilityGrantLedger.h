#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "GYAbilityGrantLedger.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;
struct FGameplayEffectSpec;

USTRUCT()
struct GY_API FGYGrantedLooseTag
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 Count = 1;

	UPROPERTY()
	EGameplayTagReplicationState RepState = EGameplayTagReplicationState::None;
};

USTRUCT()
struct GY_API FGYAbilityGrantRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGYGrantedLooseTag> LooseTags;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> AttributeSets;
};

USTRUCT()
struct GY_API FGYAbilityGrantLedger
{
	GENERATED_BODY()

	bool HasSource(FGameplayTag Source) const;

	void AddLooseTag(UAbilitySystemComponent* ASC, FGameplayTag Source, FGameplayTag Tag,
		int32 Count = 1, EGameplayTagReplicationState TagRepState = EGameplayTagReplicationState::None);

	FActiveGameplayEffectHandle ApplyEffectSpec(UAbilitySystemComponent* ASC, FGameplayTag Source,
		const FGameplayEffectSpec& Spec);

	void AdoptHandles(FGameplayTag Source, const FAbilitySetGrantedHandles& Handles);

	void RevokeSource(UAbilitySystemComponent* ASC, FGameplayTag Source);

private:
	UPROPERTY()
	TMap<FGameplayTag, FGYAbilityGrantRecord> Records;
};
