#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "BombProjectile.generated.h"

class UDecalComponent;

UCLASS()
class GY_API ABombProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	ABombProjectile();

protected:
	virtual void OnHitTarget(AActor* HitActor, const FHitResult& HitResult) override;

private:
	void Explode();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> DangerDecal;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ExplosionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float FuseTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag ExplosionCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float GravityScale = 1.f;
};
