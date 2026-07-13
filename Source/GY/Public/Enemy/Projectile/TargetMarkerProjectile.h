#pragma once

#include "CoreMinimal.h"
#include "Enemy/Projectile/ProjectileBase.h"
#include "TargetMarkerProjectile.generated.h"

UCLASS()
class GY_API ATargetMarkerProjectile : public AProjectileBase
{
	GENERATED_BODY()
public:
	ATargetMarkerProjectile();

	void LaunchHoming(AActor* InInstigator, AActor* InTarget, float InMaxTravelDistance, float InSpeed);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;
	virtual void LifeSpanExpired() override;

	void NotifyArrived(const FVector& Location);

	// 타겟 도착 판정 반경 (XY 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Marker")
	float ArriveTolerance = 50.f;

	// 초당 회전 제한(도). 낮을수록 완만하게 따라간다
	UPROPERTY(EditDefaultsOnly, Category = "Marker")
	float TurnRateDeg = 120.f;

	TWeakObjectPtr<AActor> TargetActor;
	float MaxTravelDistance = 500.f;
	float Speed = 0.f;
	float TraveledDistance = 0.f;
	bool bNotified = false;
};
