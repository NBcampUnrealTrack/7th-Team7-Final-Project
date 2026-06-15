#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "BossDataAsset.generated.h"

class AGYEnemyCharacterBase;

/**
 * 보스가 소환 가능한 잡몹 1종 정의.
 */
USTRUCT(BlueprintType)
struct FBossSummonEntry
{
	GENERATED_BODY()

	/** 어떤 종류의 잡몹인지 식별하는 EnemyType */
	UPROPERTY(EditDefaultsOnly, Category = "Summon",
		meta = (ToolTip = "소환할 잡몹 식별자. 같은 EnemyType 은 보스당 1개만 등록 가능."))
	EEnemyType EnemyType = EEnemyType::None;

	/** 잡몹 본체의 DataAsset (소프트 참조 → 비동기 프리로드) */
	UPROPERTY(EditDefaultsOnly, Category = "Summon",
		meta = (ToolTip = "잡몹 스탯/비주얼/AI 를 정의한 DataAsset. 보스 로드 시 비동기 프리로드 후 캐시된다."))
	TSoftObjectPtr<UEnemyDataAsset> DataAsset;

	/** 잡몹 액터 클래스 (소프트 참조 → 비동기 프리로드) */
	UPROPERTY(EditDefaultsOnly, Category = "Summon",
		meta = (ToolTip = "스폰될 잡몹 액터 클래스. DataAsset 과 함께 비동기 프리로드된다."))
	TSoftClassPtr<AGYEnemyCharacterBase> ActorClass;

	/**
	 * 잡몹이 받은 데미지를 보스에게 transfer 하는 비율.
	 *
	 * 예: 1.0 = 잡몹에게 들어온 데미지 100% 가 보스에게도 동일 적용 (체력 출혈).
	 * 0.0 이면 transfer 비활성. RegisterMinion 시점에 보스가 이 값을 캐싱한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Summon", meta = (ClampMin = "0.0",
		ToolTip = "잡몹이 받는 데미지를 보스 체력에 옮기는 비율. 1.0 이면 동일 데미지가 보스에게도 적용된다."))
	float HealthBleedRatio = 1.f;
};

/**
 * 보스 전용 DataAsset.
 *
 * 일반 적 DataAsset(스탯/비주얼/AI/스킬셋) 위에 보스 고유 정의를 얹는다.
 *
 * - NormalPatterns        : 평상시 회전 패턴 풀
 * - PhaseTriggers         : 체력 임계 기반 페이즈 전환 정의
 * - SummonableEnemies     : 보스가 소환 가능한 잡몹 종류
 *
 * 페이즈 컨텐츠(연출 + 순차 시퀀스 + Entry/Exit 효과)는 각 PhaseTrigger.PhaseAbilityClass
 * (UGYBossPhaseAbility 상속 BP) 안에 정의되며, 이 DataAsset 에는 트리거만 둔다.
 */
UCLASS()
class GY_API UBossDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()
public:
	/** 체력 임계 기반 페이즈 전환 정의 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase",
		meta = (ToolTip = "체력 비율(HealthRatio) 이하로 떨어지는 순간 1회 발동될 PhaseAbility 들의 목록. 여러 개를 두면 보스가 여러 페이즈를 거친다."))
	TArray<FBossPhaseTrigger> PhaseTriggers;

	/** 페이즈와 무관하게 평상시에 반복 회전될 패턴 풀 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Patterns",
		meta = (ToolTip = "페이즈와 무관하게 평상시 반복 회전될 어빌리티 풀. PatternSelector 가 거리/쿨다운/가중치로 무작위 선택해 발동한다."))
	TArray<FBossPatternEntry> NormalPatterns;

	/** 보스가 페이즈/어빌리티 통해 소환 가능한 잡몹 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon",
		meta = (ToolTip = "보스가 SummonAdds 같은 어빌리티로 소환할 수 있는 잡몹 종류. 보스 로드 시점에 모두 비동기 프리로드된다."))
	TArray<FBossSummonEntry> SummonableEnemies;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Movement",
	meta = (ToolTip = "true면 보스가 위치 이동 불가. ChaseTargetTask 즉시 통과, CharacterMovement 비활성화. 회전은 그대로."))
	bool bIsStationary = false;
};
