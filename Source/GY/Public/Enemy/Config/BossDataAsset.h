#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "BossDataAsset.generated.h"

class AGYEnemyCharacterBase;

USTRUCT(BlueprintType)
struct FBossPhaseSetup
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FName PhaseId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	bool bOverrideVisual = false;

	UPROPERTY(EditDefaultsOnly, Category = "Visual", meta = (EditCondition = "bOverrideVisual"))
	FEnemyVisualConfig VisualOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bOverrideAnimation = false;

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (EditCondition = "bOverrideAnimation"))
	FEnemyAnimationConfig AnimationOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Patterns")
	TArray<FBossPatternEntry> PhasePatterns;
};

USTRUCT(BlueprintType)
struct FBossSummonEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TSoftObjectPtr<UEnemyDataAsset> DataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TSoftClassPtr<AGYEnemyCharacterBase> ActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Summon", meta = (ClampMin = "0.0"))
	float HealthBleedRatio = 1.f;
};

UCLASS()
class GY_API UBossDataAsset : public UEnemyDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	TArray<FBossPhaseTrigger> PhaseTriggers;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	TArray<FBossPhaseSetup> PhaseSetups;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	FName InitialPhaseId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Patterns")
	TArray<FBossPatternEntry> NormalPatterns;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	TArray<FBossSummonEntry> SummonableEnemies;

	const FBossPhaseSetup* FindPhaseSetup(FName PhaseId) const;
	const FBossPhaseSetup* GetInitialPhaseSetup() const;
};
