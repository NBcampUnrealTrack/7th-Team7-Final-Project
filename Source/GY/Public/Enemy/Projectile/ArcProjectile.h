#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "ArcProjectile.generated.h"

UCLASS()
class GY_API AArcProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	AArcProjectile();

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float GravityScale = 1.f;
};
