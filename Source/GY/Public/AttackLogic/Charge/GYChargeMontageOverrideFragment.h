#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "GYChargeMontageOverrideFragment.generated.h"

// GE 인젝션 시 차지 몽타주 프래그먼트를 교체할 오버라이드 데이터
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeMontageOverrideFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeMontageOverrideFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MontageOverride")
	TMap<FGameplayTag, FGYChargeMontageSet> MontageAnimSets;
};
