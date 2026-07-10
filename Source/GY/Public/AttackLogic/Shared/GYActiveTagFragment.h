#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYActiveTagFragment.generated.h"

// 보유 태그가 RequiredTags를 모두 포함해야 선택됨. RequiredTags가 비어있으면 기본값(폴백)으로 사용
USTRUCT(BlueprintType)
struct FGYActiveTagEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer RequiredTags;

	// 선택되면 어빌리티 활성 동안 부여할 태그들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer TagsToApply;
};

// 활성 동안 부여할 태그를 무기별로 보유 (무기마다 다른 태그 조합)
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYActiveTagFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYActiveTagFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActiveTag")
	TArray<FGYActiveTagEntry> TagSets;

	// 보유 태그와 가장 일치하는 태그 세트 반환, 없으면 기본값(태그 없는 항목)
	const FGameplayTagContainer* GetBestMatchingTags(const FGameplayTagContainer& OwnedTags) const;
};
