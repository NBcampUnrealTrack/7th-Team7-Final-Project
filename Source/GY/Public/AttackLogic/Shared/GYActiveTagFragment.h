#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYActiveTagFragment.generated.h"

// 활성 동안 부여할 태그를 무기 태그별로 보유
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYActiveTagFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYActiveTagFragment();

	// 무기 태그 → 부여할 태그들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActiveTag")
	TMap<FGameplayTag, FGameplayTagContainer> TagSets;

	// 보유 태그와 가장 일치하는 태그 세트 반환, 없으면 기본값(태그 없는 항목)
	const FGameplayTagContainer* GetBestMatchingTags(const FGameplayTagContainer& OwnedTags) const;
};
