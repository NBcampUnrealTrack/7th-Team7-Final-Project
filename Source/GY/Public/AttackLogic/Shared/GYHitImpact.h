#pragma once

#include "CoreMinimal.h"
#include "GYHitImpact.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYHitImpact
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StaggerAmount = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StunAmount = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag HitFXCueTag;
};
