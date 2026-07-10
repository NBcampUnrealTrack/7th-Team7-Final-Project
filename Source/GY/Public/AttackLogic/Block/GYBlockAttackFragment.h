#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AttackLogic/Shared/GYHitImpact.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYBlockAttackFragment.generated.h"

// 막기 추가입력 단일 변형 데이터 (몽타주 + 콜리전 + 히트 임팩트 묶음)
USTRUCT(BlueprintType)
struct FGYBlockAttackSet
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
struct FGYBlockAttackEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGYBlockAttackSet Set;
};

// 막기 추가입력에 사용할 몽타주·콜리전·히트값을 무기별로 보유 (무기마다 다른 모션/수치)
UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYBlockAttackFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYBlockAttackFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlockAttack")
	TArray<FGYBlockAttackEntry> AttackSets;

	// 막기 피격 후 추가입력 가능 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlockAttack", meta = (ClampMin = "0.1", Units = "s"))
	float WindowTimeout = 2.0f;

	// 보유 태그와 가장 일치하는 세트 반환, 없으면 기본값(태그 없는 항목)
	const FGYBlockAttackSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
