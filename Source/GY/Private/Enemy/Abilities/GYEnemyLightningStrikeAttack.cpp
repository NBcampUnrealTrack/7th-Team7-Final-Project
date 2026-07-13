#include "Enemy/Abilities/GYEnemyLightningStrikeAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Projectile/AreaImpactProjectile.h"
#include "Enemy/Projectile/TargetMarkerProjectile.h"
#include "GameFramework/Pawn.h"

void UGYEnemyLightningStrikeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bMarkerArrived = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive()) return;

	UAbilityTask_WaitGameplayEvent* MarkerTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_Enemy_Marker_Arrived, nullptr, false);
	MarkerTask->EventReceived.AddDynamic(this, &UGYEnemyLightningStrikeAttack::OnMarkerArrived);
	MarkerTask->ReadyForActivation();
}

void UGYEnemyLightningStrikeAttack::OnLaunchProjectile(FGameplayEventData Payload)
{
	++WeaponWindowIndex;

	if (ComboIndex == StartStep)
	{
		SpawnMarker();
	}
	else if (ComboIndex == StrikeStep)
	{
		SpawnStrikeProjectile();
	}
}

void UGYEnemyLightningStrikeAttack::SpawnMarker()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !MarkerClass) return;

	AActor* Target = nullptr;
	if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Pawn->GetController()))
	{
		Target = AI->GetTargetActor();
	}

	FVector LaunchPos = Pawn->GetActorLocation();
	FHitResult GroundHit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(LightningMarkerGround), false, Pawn);
	if (GetWorld()->LineTraceSingleByChannel(GroundHit,
		LaunchPos, LaunchPos - FVector(0.f, 0.f, 1000.f), ECC_Visibility, TraceParams))
	{
		LaunchPos.Z = GroundHit.ImpactPoint.Z + MarkerGroundOffset;
	}

	FVector Dir = Pawn->GetActorForwardVector();
	if (Target)
	{
		FVector ToTarget = Target->GetActorLocation() - LaunchPos;
		ToTarget.Z = 0.f;
		if (ToTarget.Normalize())
		{
			Dir = ToTarget;
		}
	}

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.Instigator = Pawn;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATargetMarkerProjectile* Marker = GetWorld()->SpawnActor<ATargetMarkerProjectile>(
		MarkerClass, LaunchPos, Dir.Rotation(), Params);
	if (Marker)
	{
		Marker->LaunchHoming(Pawn, Target, MaxMarkerDistance, MarkerSpeed);
	}
}

void UGYEnemyLightningStrikeAttack::OnMarkerArrived(FGameplayEventData Payload)
{
	if (bMarkerArrived) return;
	bMarkerArrived = true;

	FVector ArrivedLoc = FVector::ZeroVector;
	if (Payload.TargetData.Num() > 0)
	{
		if (const FHitResult* Hit = Payload.TargetData.Get(0)->GetHitResult())
		{
			ArrivedLoc = Hit->Location;
		}
	}

	StrikeLocation = ArrivedLoc;
	FHitResult GroundHit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(LightningStrikeGround), false, GetAvatarActorFromActorInfo());
	if (GetWorld()->LineTraceSingleByChannel(GroundHit,
		ArrivedLoc + FVector(0.f, 0.f, 200.f), ArrivedLoc - FVector(0.f, 0.f, 2000.f),
		ECC_Visibility, TraceParams))
	{
		StrikeLocation = GroundHit.ImpactPoint;
	}

	if (WarningActorClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetAvatarActorFromActorInfo();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AActor>(WarningActorClass, StrikeLocation, FRotator::ZeroRotator, Params);
	}

	PlayComboMontage(StrikeStep);
}

void UGYEnemyLightningStrikeAttack::SpawnStrikeProjectile()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !StrikeClass) return;

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.Instigator = Pawn;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAreaImpactProjectile* Strike = GetWorld()->SpawnActor<AAreaImpactProjectile>(
		StrikeClass, StrikeLocation, FRotator::ZeroRotator, Params);
	if (!Strike) return;

	Strike->Detonate(Pawn);
}

void UGYEnemyLightningStrikeAttack::OnComboMontageEnded()
{
	if (ComboIndex == StartStep)
	{
		PlayComboMontage(LoopStep);
		return;
	}

	if (ComboIndex == LoopStep)
	{
		PlayComboMontage(bMarkerArrived ? StrikeStep : LoopStep);
		return;
	}

	Super::OnComboMontageEnded();
}
