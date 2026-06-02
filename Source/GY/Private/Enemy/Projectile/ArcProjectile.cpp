#include "Enemy/Projectile/ArcProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"

AArcProjectile::AArcProjectile()
{
	ProjectileMovement->ProjectileGravityScale = GravityScale;
}
