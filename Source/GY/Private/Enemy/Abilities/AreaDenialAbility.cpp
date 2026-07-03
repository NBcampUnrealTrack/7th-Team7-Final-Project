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
#include "Enemy/Hazard/PoisonZoneActor.h"

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
	if (PoisonZoneClass)
	{
		// Notify가 보내는 zone 스폰 이벤트 대기
		UAbilityTask_WaitGameplayEvent* ZoneTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GYGameplayTags::Event_Enemy_SpawnAreaDenialZones, nullptr, false);
		ZoneTask->EventReceived.AddDynamic(this, &UAreaDenialAbility::OnSpawnZonesEvent);
		ZoneTask->ReadyForActivation();
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
	if (!World) return;

	// WarningDuration <= 0 : 경고 표시 없이 즉발 낙하
	const bool bImmediate = (WarningDuration <= 0.f);

	// 경고 모드(>0)인데 경고 액터가 없으면 기존처럼 종료
	if (!bImmediate && !HazardActorClass) return;

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
		case EHazardPlacementMode::AtBoss:
			{
				Candidate = Origin;
			}
			break;
		}

		if (!IsSpacingOK(Candidate, Placed)) continue;

		// 지면 확인 (경고를 안 뿌려도 낙하 지점 유효성은 검사)
		{
			const FVector TraceStart = Candidate + FVector(0.f, 0.f, GroundTraceHeightAbove);
			const FVector TraceEnd   = Candidate - FVector(0.f, 0.f, GroundTraceHeightBelow);

			FHitResult GroundHit;
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(AreaDenialGround), false);
			if (BossActor) TraceParams.AddIgnoredActor(BossActor);

			if (!World->LineTraceSingleByChannel(
					GroundHit, TraceStart, TraceEnd, GroundTraceChannel, TraceParams))
			{
				continue;
			}
		}

		// 경고 액터는 WarningDuration > 0 이고 클래스가 있을 때만 스폰
		if (!bImmediate && HazardActorClass)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Params.Owner = BossActor;

			const FVector SpawnLoc = Candidate + FVector(0.f, 0.f, HazardGroundOffset);
			if (AActor* HazardActor = World->SpawnActor<AActor>(HazardActorClass, SpawnLoc, FRotator::ZeroRotator, Params))
			{
				HazardActor->SetLifeSpan(WarningDuration);
			}
		}

		Placed.Add(Candidate);
		++Spawned;
	}

	if (Placed.Num() == 0) return;

	CachedImpactLocations = Placed;

	if (ProjectileClass)
	{
		if (bImmediate)
		{
			// 즉발: 타이머 없이 바로 낙하 (SetTimer rate=0 무효화 함정 회피)
			SpawnImpactProjectiles();
		}
		else
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, this,
				&UAreaDenialAbility::SpawnImpactProjectiles, WarningDuration, false);
		}
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
	HitContext.bGivesParriedReaction = false;
	UGYCombatStatics::ApplyHitImpact(HitContext);
}

void UAreaDenialAbility::SpawnPoisonZones()
{
	UWorld* World = GetWorld();
	if (!World || !PoisonZoneClass) return;

	AActor* Boss = GetAvatarActorFromActorInfo();
	if (!Boss) return;

	for (const FVector& Loc : CachedImpactLocations)
	{
		FActorSpawnParameters Params;
		Params.Owner = Boss;
		Params.Instigator = Cast<APawn>(Boss);
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APoisonZoneActor* Zone = World->SpawnActor<APoisonZoneActor>(
			PoisonZoneClass, Loc, FRotator::ZeroRotator, Params);

		if (Zone) Zone->Initialize(Boss);
	}
	CachedImpactLocations.Reset();
}

void UAreaDenialAbility::OnSpawnZonesEvent(FGameplayEventData Payload)
{
	SpawnPoisonZones();
}

