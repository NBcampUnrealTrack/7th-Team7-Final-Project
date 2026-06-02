#pragma once

#include "CoreMinimal.h"
#include "ArcProjectile.h"
#include "BombProjectile.generated.h"

class UDecalComponent;

UCLASS()
class GY_API ABombProjectile : public AArcProjectile
{
	GENERATED_BODY()

public:
	ABombProjectile();

protected:
	virtual void OnHitTarget(AActor* HitActor, const FHitResult& HitResult) override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> DangerDecal;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ExplosionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float FuseTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag ExplosionCueTag;

private:
	void Explode();
};
