#pragma once

#include "CoreMinimal.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "Engine/DataAsset.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "BossDataAsset.generated.h"

UCLASS()
class GY_API UBossDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	TArray<FBossPhaseTrigger> PhaseTriggers;

	UPROPERTY(EditDefaultsOnly, Category ="Boss|Patterns")
	TArray<FBossPatternEntry> NormalPatterns;
};
