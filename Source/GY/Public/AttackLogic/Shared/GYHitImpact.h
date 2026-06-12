#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GYHitImpact.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYHitImpact
{
	GENERATED_BODY()

	// 이 타격의 공격 타입(Ability.Attack.Combo/Charge). 조건부 인첸트(약/강공) 매칭에 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Ability.Attack"))
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StaggerAmount = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StunAmount = 10.f;
};
