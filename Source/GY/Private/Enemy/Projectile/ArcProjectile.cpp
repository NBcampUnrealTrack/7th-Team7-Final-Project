#include "Enemy/Projectile/ArcProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"

void AArcProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ProjectileMovement->ProjectileGravityScale = GravityScale;
}
