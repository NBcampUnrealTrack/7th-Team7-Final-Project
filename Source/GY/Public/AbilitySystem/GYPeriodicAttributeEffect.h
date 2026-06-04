#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GYPeriodicAttributeEffect.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GY_API UGYPeriodicAttributeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGYPeriodicAttributeEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY")
	float CombatStartDelay = 0.f;
};
