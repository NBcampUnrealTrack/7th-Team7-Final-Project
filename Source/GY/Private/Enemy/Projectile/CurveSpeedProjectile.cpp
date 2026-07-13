#include "Enemy/Projectile/CurveSpeedProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"


ACurveSpeedProjectile::ACurveSpeedProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ACurveSpeedProjectile::Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed)
{
	Super::Launch(InInstigator, InDirection, InSpeed);

	BaseSpeed = InSpeed;
	ElapsedTime = 0.f;

	if (SpeedCurve)
	{
		ProjectileMovement->MaxSpeed = 0.f;
		SetActorTickEnabled(true);
	}
}

void ACurveSpeedProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!SpeedCurve || !ProjectileMovement) return;

	ElapsedTime += DeltaTime;

	const float CurveTime = (CurveDuration > 0.f) ? FMath::Clamp(ElapsedTime / CurveDuration, 0.f, 1.f) : ElapsedTime;

	const FVector Dir = ProjectileMovement->Velocity.GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		const float SpeedMul = FMath::Max(SpeedCurve->GetFloatValue(CurveTime), 0.f);
		ProjectileMovement->Velocity = Dir * BaseSpeed * SpeedMul;
	}
}

