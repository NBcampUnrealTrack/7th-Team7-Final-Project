#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "AreaImpactProjectile.generated.h"

class UNiagaraSystem;

UCLASS()
class GY_API AAreaImpactProjectile : public AProjectileBase
{
	GENERATED_BODY()

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

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactCueTag;

	/** 소멸 시 재생할 Niagara 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	bool bImpacted = false;
};
