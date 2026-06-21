#pragma once

#include "CoreMinimal.h"
#include "Enemy/Component/EnemyBootstrapComponent.h"
#include "Enemy/Config/BossDataAsset.h"
#include "BossBootstrapComponent.generated.h"

class AGYEnemyCharacterBase;

USTRUCT()
struct FBossCachedSummonable
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UEnemyDataAsset> DataAsset;

	UPROPERTY()
	TSubclassOf<AGYEnemyCharacterBase> ActorClass;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UBossBootstrapComponent : public UEnemyBootstrapComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Boss|Data")
	UBossDataAsset* GetBossDataAsset() const { return Cast<UBossDataAsset>(GetDataAsset()); }

	bool GetSummonable(EEnemyType Type, FBossCachedSummonable& Out) const;

protected:
	virtual void ApplyAllConfigs() override;
	virtual void GrantDefaultAbilities() override;
	virtual void RequestExtraPreload() override;

	void HandleSummonablesLoaded();

	UPROPERTY(Transient)
	TMap<EEnemyType, FBossCachedSummonable> SummonCache;
};
