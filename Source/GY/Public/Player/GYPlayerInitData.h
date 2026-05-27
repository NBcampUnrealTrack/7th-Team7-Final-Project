#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GYPlayerInitData.generated.h"

UCLASS()
class GY_API UGYPlayerInitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float Attack = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float Defense = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Strength = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Dexterity = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Additional", meta = (ClampMin = "0.0"))
	float MaxStagger = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Additional", meta = (ClampMin = "0.0"))
	float MaxStun = 100.f;
};
