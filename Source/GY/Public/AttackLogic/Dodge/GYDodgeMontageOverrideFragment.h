#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "GYDodgeMontageOverrideFragment.generated.h"

// GE 인젝션 시 회피 몽타주 프래그먼트를 교체할 오버라이드 데이터
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeMontageOverrideFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeMontageOverrideFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MontageOverride")
	TMap<FGameplayTag, FGYDodgeMontageSet> MontageAnimSets;
};
