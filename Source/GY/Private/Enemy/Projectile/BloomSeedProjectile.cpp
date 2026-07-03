#include "Enemy/Projectile/BloomSeedProjectile.h"

#include "Enemy/Hazard/PoisonZoneActor.h"
#include "GameFramework/Pawn.h"
#include "Logging/GYLogManager.h"

ABloomSeedProjectile::ABloomSeedProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABloomSeedProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	if (!HasAuthority())
	{
		Destroy();
		return;
	}

	if (!ZoneClass || !InstigatorActor.IsValid())
	{
		GY_WARN(AI, ESK, "BloomSeed: ZoneClass 미설정 또는 Instigator 무효");
		Destroy();
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = InstigatorActor.Get();
	Params.Instigator = Cast<APawn>(InstigatorActor.Get());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APoisonZoneActor* Zone = GetWorld()->SpawnActor<APoisonZoneActor>(
		ZoneClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, Params);
	if (Zone)
	{
		Zone->Initialize(InstigatorActor.Get());
		GY_LOG(AI, ESK, "BloomSeed: 장판 스폰 (Loc=%s)", *ImpactResult.ImpactPoint.ToString());
	}
	Destroy();
}

void ABloomSeedProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}


