#include "Enemy/Abilities/ArenaImpactAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/Actor/AreaWarningActor.h"
#include "Enemy/Projectile/AreaImpactProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UArenaImpactAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* Boss = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!World || !Boss || !ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_Enemy_WeaponTrace_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &UArenaImpactAbility::OnProjectileHit);
	HitTask->ReadyForActivation();

	PlayAttackMontage();

	FVector ImpactLoc = AAreaWarningActor::ResolveArenaCenter(
		World, ArenaCenterTag, Boss->GetActorLocation());
	if (!AnchorActorTag.IsNone())
	{
		TArray<AActor*> Anchors;
		UGameplayStatics::GetAllActorsWithTag(World, AnchorActorTag, Anchors);
		if (Anchors.Num() > 0)
		{
			ImpactLoc = Anchors[0]->GetActorLocation();
		}
	}

	FActorSpawnParameters Params;
	Params.Owner = Boss;
	Params.Instigator = Boss;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AAreaImpactProjectile* Proj = World->SpawnActor<AAreaImpactProjectile>(
			ProjectileClass, ImpactLoc, FRotator::ZeroRotator, Params))
	{
		SpawnedProjectile = Proj;

		if (ImpactDelay > 0.f)
		{
			World->GetTimerManager().SetTimer(
				ImpactTimerHandle, this, &UArenaImpactAbility::DoImpact, ImpactDelay, false);
		}
		else
		{
			DoImpact();
		}
	}
}

void UArenaImpactAbility::DoImpact()
{
	APawn* Boss = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (SpawnedProjectile.IsValid() && Boss)
	{
		SpawnedProjectile->Detonate(Boss);
	}
}

void UArenaImpactAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ImpactTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UArenaImpactAbility::OnProjectileHit(FGameplayEventData Payload)
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
	if (HitDamageWeights.IsValidIndex(0))
	{
		HitContext.KnockbackStrength = HitDamageWeights[0].KnockbackStrength;
	}
	UGYCombatStatics::ApplyHitImpact(HitContext);
}
