#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilitySets/AbilitySetGrantedHandles.h"
#include "AbilitySet.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> Ability;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 1))
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;
};

USTRUCT(BlueprintType)
struct FAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.f;
};

USTRUCT(BlueprintType)
struct FAbilitySet_AttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet;
};

UCLASS(BlueprintType, Const)
class GY_API UAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Abilities", meta = (TitleProperty = "Ability"))
	TArray<FAbilitySet_GameplayAbility> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (TitleProperty = "GameplayEffect"))
	TArray<FAbilitySet_GameplayEffect> GrantedEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (TitleProperty = "AttributeSet"))
	TArray<FAbilitySet_AttributeSet> GrantedAttributes;

	void GiveToAbilitySystem(
		UAbilitySystemComponent* ASC,
		FAbilitySetGrantedHandles* OutHandles,
		UObject* SourceObject = nullptr) const;
};
