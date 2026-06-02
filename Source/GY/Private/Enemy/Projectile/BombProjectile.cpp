#include "Enemy/Projectile/BombProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/DecalComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"

ABombProjectile::ABombProjectile()
{
	DangerDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("DangerDecal"));
	DangerDecal->SetupAttachment(RootComponent);
	DangerDecal->SetVisibility(false);
}

void ABombProjectile::OnHitTarget(AActor* HitActor, const FHitResult& HitResult)
{
	ProjectileMovement->StopMovementImmediately();
	DangerDecal->SetVisibility(true);
	FTimerHandle FuseTimer;
	GetWorldTimerManager().SetTimer(FuseTimer, this, &ABombProjectile::Explode, FuseTime, false);
}

void ABombProjectile::Explode()
{
	if (!GetWorld()) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ExplosionRadius),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlapActor);
		if (!TargetASC) continue;

		FGameplayEventData Payload;
		Payload.Instigator = InstigatorActor.Get();
		Payload.Target = OverlapActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			InstigatorActor.Get(),
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			Payload);
	}

	if (ExplosionCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = GetActorLocation();
			ASC->ExecuteGameplayCue(ExplosionCueTag, CueParams);
		}
	}
	Destroy();
}

