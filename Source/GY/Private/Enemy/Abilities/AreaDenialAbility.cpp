#include "Enemy/Abilities/AreaDenialAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/Projectile/AreaImpactProjectile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UAreaDenialAbility::UAreaDenialAbility()
{
}

void UAreaDenialAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ProjectileClass)
	{
		UAbilityTask_WaitGameplayEvent* HitTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GYGameplayTags::Event_Enemy_WeaponTrace_Hit, nullptr, false);
		HitTask->EventReceived.AddDynamic(this, &UAreaDenialAbility::OnProjectileHit);
		HitTask->ReadyForActivation();
	}

	PlayAttackMontage();
	ExecuteAreaDenail();
}

bool UAreaDenialAbility::IsSpacingOK(const FVector& Candidate, const TArray<FVector>& Placed) const
{
	for (const FVector& Place : Placed)
	{
		if (FVector::Dist(Candidate, Place) < MinSpacing)
			return false;
	}
	return true;
}

void UAreaDenialAbility::ExecuteAreaDenail()
{
	UWorld* World = GetWorld();
	if (!World || !HazardActorClass) return;

	AActor* BossActor = GetAvatarActorFromActorInfo();
	const FVector Origin = BossActor ? BossActor->GetActorLocation() : FVector::ZeroVector;

	TArray<FVector> PlayerLocations;
	if (PlacementMode == EHazardPlacementMode::AroundPlayers)
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Pawn = *It;
			if (Pawn && Pawn->IsPlayerControlled())
			{
				PlayerLocations.Add(Pawn->GetActorLocation());
			}
		}
	}

	TArray<FVector> Placed;
	Placed.Reserve(HazardCount);
	const int32 MaxAttempts = HazardCount * 10;
	int32 Attempts = 0;
	int32 Spawned = 0;

	while (Spawned < HazardCount && Attempts < MaxAttempts)
	{
		++Attempts;
		FVector Candidate = FVector::ZeroVector;

		switch (PlacementMode)
		{
		case EHazardPlacementMode::RandomInArea:
			{
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				const float Dist = FMath::FRandRange(0.f, ArenaRadius);
				Candidate = Origin + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			}
			break;
		case EHazardPlacementMode::AroundBoss:
			{
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				Candidate = Origin + FVector(FMath::Cos(Angle) * PlacementRadius,
					FMath::Sin(Angle) * PlacementRadius, 0.f);
			}
			break;
		case EHazardPlacementMode::AroundPlayers:
			{
				if (PlayerLocations.Num() == 0) return;
				const FVector& PLoc = PlayerLocations[FMath::RandRange(0, PlayerLocations.Num() - 1)];
				const float Angle = FMath::FRandRange(0.f, 2.f * PI);
				const float Dist = FMath::FRandRange(0.f, PlacementRadius);
				Candidate = PLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			}
			break;
		}

		if (!IsSpacingOK(Candidate, Placed)) continue;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = BossActor;

		if (World->SpawnActor<AActor>(HazardActorClass, Candidate, FRotator::ZeroRotator, Params))
		{
			Placed.Add(Candidate);
			++Spawned;
		}
	}

	if (ProjectileClass && Placed.Num() > 0)
	{
		CachedImpactLocations = Placed;
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, this,
			&UAreaDenialAbility::SpawnImpactProjectiles, WarningDuration, false);
	}
}

void UAreaDenialAbility::SpawnImpactProjectiles()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass) return;

	APawn* Boss = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Boss) return;

	for (const FVector& Loc : CachedImpactLocations)
	{
		const FVector SpawnLoc = Loc + FVector(0.f, 0.f, SpawnHeight);

		FActorSpawnParameters Params;
		Params.Owner = Boss;
		Params.Instigator = Boss;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AAreaImpactProjectile* P = World->SpawnActor<AAreaImpactProjectile>(
			ProjectileClass, SpawnLoc, FRotator(-90.f, 0.f, 0.f), Params);

		if (P) P->Launch(Boss, FVector(0.f, 0.f, -1.f), DropSpeed);
	}
	CachedImpactLocations.Reset();
}

void UAreaDenialAbility::OnProjectileHit(FGameplayEventData Payload)
{
	AActor* HitActor   = const_cast<AActor*>(Payload.Target.Get());
	AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get());
	if (!HitActor || !Instigator) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !OwnerASC) return;

	float MotionMultiplier = 1.f;
	float Additive         = 0.f;
	float Stagger          = 0.f;
	float Stun             = 0.f;
	if (HitDamageWeights.IsValidIndex(0))
	{
		const FHitDamageWeight& W = HitDamageWeights[0];
		MotionMultiplier = W.Multiplicative;
		Additive         = W.Additive;
		Stagger          = W.Stagger;
		Stun             = W.Stun;
	}

	FGYHitContext HitContext;
	HitContext.SourceASC        = OwnerASC;
	HitContext.TargetASC        = TargetASC;
	HitContext.MotionMultiplier = MotionMultiplier;
	HitContext.Additive         = Additive;
	HitContext.StaggerAmount    = Stagger;
	HitContext.StunAmount       = Stun;

	UGYCombatStatics::ApplyHitImpact(HitContext);
}

