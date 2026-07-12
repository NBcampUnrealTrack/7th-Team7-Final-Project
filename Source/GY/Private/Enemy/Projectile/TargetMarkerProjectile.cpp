#include "Enemy/Projectile/TargetMarkerProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SphereComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/ProjectileMovementComponent.h"

ATargetMarkerProjectile::ATargetMarkerProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void ATargetMarkerProjectile::LaunchHoming(AActor* InInstigator, AActor* InTarget, float InMaxTravelDistance, float InSpeed)
{
	TargetActor = InTarget;
	MaxTravelDistance = InMaxTravelDistance;
	Speed = InSpeed;

	FVector Dir = GetActorForwardVector();
	if (InTarget)
	{
		FVector ToTarget = InTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		if (ToTarget.Normalize())
		{
			Dir = ToTarget;
		}
	}

	Launch(InInstigator, Dir, InSpeed);
	SetActorTickEnabled(true);
}

void ATargetMarkerProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bNotified || !HasAuthority()) return;

	FVector CurrentDir = ProjectileMovement->Velocity;
	CurrentDir.Z = 0.f;
	if (!CurrentDir.Normalize())
	{
		CurrentDir = GetActorForwardVector();
	}

	if (TargetActor.IsValid())
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (ToTarget.SizeSquared() <= FMath::Square(ArriveTolerance))
		{
			NotifyArrived(GetActorLocation());
			return;
		}

		if (ToTarget.Normalize())
		{
			const float CurrentYaw = FMath::Atan2(CurrentDir.Y, CurrentDir.X);
			const float TargetYaw = FMath::Atan2(ToTarget.Y, ToTarget.X);
			const float MaxStep = FMath::DegreesToRadians(TurnRateDeg) * DeltaSeconds;
			const float Delta = FMath::Clamp(
				FMath::FindDeltaAngleRadians(CurrentYaw, TargetYaw), -MaxStep, MaxStep);
			const float NewYaw = CurrentYaw + Delta;

			CurrentDir = FVector(FMath::Cos(NewYaw), FMath::Sin(NewYaw), 0.f);
		}
	}

	ProjectileMovement->Velocity = CurrentDir * Speed;

	TraveledDistance += Speed * DeltaSeconds;
	if (TraveledDistance >= MaxTravelDistance)
	{
		NotifyArrived(GetActorLocation());
	}
}

void ATargetMarkerProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	if (!HasAuthority()) return;
	NotifyArrived(ImpactResult.bBlockingHit ? FVector(ImpactResult.ImpactPoint) : GetActorLocation());
}

void ATargetMarkerProjectile::LifeSpanExpired()
{
	NotifyArrived(GetActorLocation());
}

void ATargetMarkerProjectile::NotifyArrived(const FVector& Location)
{
	if (bNotified) return;
	bNotified = true;

	if (InstigatorActor.IsValid())
	{
		FHitResult HitResult;
		HitResult.Location    = Location;
		HitResult.ImpactPoint = Location;

		FGameplayAbilityTargetData_SingleTargetHit* TargetData =
			new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(TargetData);

		FGameplayEventData Payload;
		Payload.Instigator = InstigatorActor.Get();
		Payload.TargetData = TargetDataHandle;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			InstigatorActor.Get(), GYGameplayTags::Event_Enemy_Marker_Arrived, Payload);
	}
	Destroy();
}
