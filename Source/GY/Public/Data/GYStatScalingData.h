#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GYStatScalingData.generated.h"

UCLASS(BlueprintType)
class GY_API UGYStatScalingData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Strength")
	float StrengthToMaxHealth = 3.f;

	UPROPERTY(EditDefaultsOnly, Category="Strength")
	float StrengthToHitResistance = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Strength")
	float StrengthToSwordAndShieldMultiplier = 0.015f;

	UPROPERTY(EditDefaultsOnly, Category="Dexterity")
	float DexterityToCriticalRate = 0.003f;

	UPROPERTY(EditDefaultsOnly, Category="Dexterity")
	float DexterityToEvasionInvincibilityTime = 0.005f;

	UPROPERTY(EditDefaultsOnly, Category="Dexterity")
	float DexterityToSwordAndShieldMultiplier = 0.015f;
};
