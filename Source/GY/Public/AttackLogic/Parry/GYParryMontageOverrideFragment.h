#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Parry/GYParryMontageFragment.h"
#include "GYParryMontageOverrideFragment.generated.h"

// GE 인젝션 시 패리 몽타주 프래그먼트를 교체할 오버라이드 데이터
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYParryMontageOverrideFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYParryMontageOverrideFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MontageOverride")
	TMap<FGameplayTag, FGYParryMontageSet> MontageAnimSets;
};
