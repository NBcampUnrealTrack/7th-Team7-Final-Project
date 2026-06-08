#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GYStatScalingData.generated.h"

USTRUCT(BlueprintType)
struct FWeaponStyleScalingFactors
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Scaling")
	float StrengthFactor = 0.015f;

	UPROPERTY(EditDefaultsOnly, Category="Scaling")
	float DexterityFactor = 0.015f;
};

UCLASS(BlueprintType)
class GY_API UGYStatScalingData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Strength")
	float StrengthToMaxHealth = 3.f;

	UPROPERTY(EditDefaultsOnly, Category="Strength")
	float StrengthToHitResistance = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Dexterity")
	float DexterityToCriticalRate = 0.003f;

	UPROPERTY(EditDefaultsOnly, Category="Dexterity")
	float DexterityToEvasionInvincibilityTime = 0.005f;

	UPROPERTY(EditDefaultsOnly, Category="WeaponStyle", meta=(Categories="Combat.Style"))
	TMap<FGameplayTag, FWeaponStyleScalingFactors> WeaponStyleFactors;
};
