#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "AreaImpactProjectile.generated.h"

class UNiagaraSystem;

UCLASS()
class GY_API AAreaImpactProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	void Detonate(AActor* InInstigator);

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	virtual void OnProjectileMovementStop(const FHitResult& ImpactResult) override;
private:
	void TriggerImpact(const FVector& ImpactLocation);

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Impact")
	float ImpactRadius = 300.f;

	// Detonate 후 액터 유지 시간. 부착된 이펙트 재생 길이에 맞춘다
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Impact")
	float PostImpactLifeTime = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactCueTag;

	bool bImpacted = false;
};
