#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GYRegenDelayEffect.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GY_API UGYRegenDelayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGYRegenDelayEffect();
};
