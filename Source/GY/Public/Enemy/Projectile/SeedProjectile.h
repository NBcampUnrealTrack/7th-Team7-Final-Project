#pragma once

#include "CoreMinimal.h"
#include "ArcProjectile.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "SeedProjectile.generated.h"

UCLASS()
class GY_API ASeedProjectile : public AArcProjectile
{
	GENERATED_BODY()

protected:
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;

	UPROPERTY(EditDefaultsOnly, Category = "Seed")
	EEnemyType TentacleEnemyType = EEnemyType::None;
};
