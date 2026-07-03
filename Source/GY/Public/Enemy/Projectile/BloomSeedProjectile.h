#pragma once

#include "CoreMinimal.h"
#include "AreaImpactProjectile.h"
#include "BloomSeedProjectile.generated.h"

class APoisonZoneActor;

UCLASS()
class GY_API ABloomSeedProjectile : public AAreaImpactProjectile
{
	GENERATED_BODY()

public:
	ABloomSeedProjectile();

	/** 지면 착지 시 장판 스폰 */
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;

	/** 공중에서 플레이어와 닿아도 즉발 안되게 */
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	/** 착지 지점에 생성할 장판 */
	UPROPERTY(EditDefaultsOnly, Category = "BloomSeed")
	TSubclassOf<APoisonZoneActor> ZoneClass;
};
