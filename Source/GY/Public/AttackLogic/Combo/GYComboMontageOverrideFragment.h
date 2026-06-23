#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Combo/GYComboMontageFragment.h"
#include "GYComboMontageOverrideFragment.generated.h"

// GE 인젝션 시 콤보 몽타주 프래그먼트를 교체할 오버라이드 데이터
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYComboMontageOverrideFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYComboMontageOverrideFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MontageOverride")
	TMap<FGameplayTag, FGYComboMontageSet> MontageAnimSets;
};
