#include "Enemy/Projectile/AreaImpactProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Core/GameplayTags/EventTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Logging/GYLogManager.h"

void AAreaImpactProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GY_LOG(AI, ESK, "AreaImpactProjectile::Overlap - Other=%s, Instigator=%s, bImpacted=%d",
		*GetNameSafe(OtherActor),
		*GetNameSafe(InstigatorActor.Get()),
		bImpacted ? 1 : 0);

	if (!HasAuthority()) return;
	if (bImpacted) return;
	if (!InstigatorActor.IsValid()) return;
	if (!OtherActor || OtherActor == this) return;
	if (OtherActor == InstigatorActor.Get()) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled()) return;

	bImpacted = true;
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	TriggerImpact(GetActorLocation());
	Destroy();
}

void AAreaImpactProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	GY_LOG(AI, ESK, "AreaImpact::Stop - Hit=%s, Point=%s",
		*GetNameSafe(ImpactResult.GetActor()),
		*ImpactResult.ImpactPoint.ToString());

	if (!HasAuthority()) return;
	if (bImpacted) return;

	bImpacted = true;
	TriggerImpact(ImpactResult.ImpactPoint);
	Destroy();
}

void AAreaImpactProjectile::TriggerImpact(const FVector& ImpactLocation)
{
	UWorld* World = GetWorld();
	if (!World || !InstigatorActor.IsValid()) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(InstigatorActor.Get());

	World->OverlapMultiByChannel(
		Overlaps,
		ImpactLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ImpactRadius),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Hit = Overlap.GetActor();
		if (!Hit) continue;

		APawn* Pawn = Cast<APawn>(Hit);
		if (!Pawn || !Pawn->IsPlayerControlled()) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit);
		if (!TargetASC) continue;

		FHitResult HitResult;
		HitResult.ImpactPoint  = ImpactLocation;
		HitResult.ImpactNormal = FVector::UpVector;
		HitResult.Location     = ImpactLocation;

		FGameplayAbilityTargetData_SingleTargetHit* TargetData =
			new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(TargetData);

		FGameplayEventData Payload;
		Payload.Instigator = InstigatorActor.Get();
		Payload.Target     = Hit;
		Payload.TargetData = TargetDataHandle;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			InstigatorActor.Get(),
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			Payload);
	}

	if (ImpactCueTag.IsValid())
	{
		UAbilitySystemComponent* SourceASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
		if (SourceASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = ImpactLocation;
			CueParams.Normal   = FVector::UpVector;
			SourceASC->ExecuteGameplayCue(ImpactCueTag, CueParams);
		}
	}
}
