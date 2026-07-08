#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GYEnemyCooldownEffect.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GY_API UGYEnemyCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGYEnemyCooldownEffect();
};
