#include "Enemy/Projectile/SlashProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Core/GYCollisionChannels.h"
#include "GameFramework/ProjectileMovementComponent.h"

ASlashProjectile::ASlashProjectile()
{
	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(CollisionComponent);
	HitBox->SetBoxExtent(FVector(30.f, 120.f, 60.f));
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBox->SetCollisionObjectType(ECC_EnemyProjectile);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitBox->OnComponentBeginOverlap.AddDynamic(this, &ASlashProjectile::OnProjectileOverlap);

	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	ProjectileMovement->bRotationFollowsVelocity = false;
}

void ASlashProjectile::Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed)
{
	Super::Launch(InInstigator, InDirection, InSpeed);
	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
