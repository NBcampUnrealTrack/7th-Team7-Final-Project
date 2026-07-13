#include "Enemy/Projectile/CurseKnifeProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Enemy/Hazard/PoisonZoneActor.h"
#include "GameFramework/Pawn.h"

void ACurseKnifeProjectile::OnHitTarget(AActor* HitActor, const FHitResult& HitResult)
{
	Super::OnHitTarget(HitActor, HitResult);

	if (!CurseMarkEffectClass || !InstigatorActor.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(InstigatorActor.Get(), this);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(CurseMarkEffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ACurseKnifeProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	if (!HasAuthority())
	{
		Destroy();
		return;
	}

	if (ZoneClass && InstigatorActor.IsValid())
	{
		FActorSpawnParameters Params;
		Params.Owner = InstigatorActor.Get();
		Params.Instigator = Cast<APawn>(InstigatorActor.Get());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		if (APoisonZoneActor* Zone = GetWorld()->SpawnActor<APoisonZoneActor>(
			ZoneClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, Params))
		{
			Zone->Initialize(InstigatorActor.Get());
		}
	}

	Destroy();
}
