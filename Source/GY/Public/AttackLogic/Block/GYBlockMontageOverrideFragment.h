#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "GYBlockMontageOverrideFragment.generated.h"

// GE 인젝션 시 블록 몽타주 프래그먼트를 교체할 오버라이드 데이터
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYBlockMontageOverrideFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYBlockMontageOverrideFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MontageOverride")
	TMap<FGameplayTag, FGYBlockMontageSet> MontageAnimSets;
};
