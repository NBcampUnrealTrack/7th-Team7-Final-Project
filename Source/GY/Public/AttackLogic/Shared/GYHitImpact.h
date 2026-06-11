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
	float StaggerDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StunDamage = 10.f;
};
