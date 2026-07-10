#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AttackLogic/Shared/GYHitImpact.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYParryCounterFragment.generated.h"

// 패리 추가입력 단일 변형 데이터 (몽타주 + 콜리전 + 히트 임팩트 묶음)
USTRUCT(BlueprintType)
struct FGYParryCounterSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGYCollisionShapeData CollisionData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGYHitImpact HitImpact;
};

// 보유 태그가 RequiredTags를 모두 포함해야 선택됨. RequiredTags가 비어있으면 기본값(폴백)으로 사용
USTRUCT(BlueprintType)
struct FGYParryCounterEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGYParryCounterSet Set;
};

// 패리 추가입력에 사용할 몽타주·콜리전·히트값을 무기별로 보유 (무기마다 다른 모션/수치)
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYParryCounterFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYParryCounterFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParryCounter")
	TArray<FGYParryCounterEntry> AttackSets;

	// 패리 성공 후 추가입력 가능 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParryCounter", meta = (ClampMin = "0.1", Units = "s"))
	float WindowTimeout = 1.5f;

	// 보유 태그와 가장 일치하는 세트 반환, 없으면 기본값(태그 없는 항목)
	const FGYParryCounterSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
