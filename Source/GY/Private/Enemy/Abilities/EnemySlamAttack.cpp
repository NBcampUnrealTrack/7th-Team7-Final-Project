#include "Enemy/Abilities/EnemySlamAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Core/GameplayTags/EventTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Enemy/GYEnemyAIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UEnemySlamAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ResolveTargetLocation(TriggerEventData);

	const FVector Self = Avatar->GetActorLocation();
	const FVector Flat = FVector(TargetLocation.X - Self.X, TargetLocation.Y - Self.Y, 0.f);
	const float Distance = Flat.Size();

	if (Distance < KINDA_SMALL_NUMBER || HorizontalSpeed < KINDA_SMALL_NUMBER)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FRotator JumpRot = Flat.Rotation();
	const float Duration = Distance / HorizontalSpeed;

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, PlayRate, NAME_None, true);
	MontageTask->ReadyForActivation();

	if (UCharacterMovementComponent* MovementComponent = Avatar->GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
	}

	UAbilityTask_ApplyRootMotionJumpForce* Jump =
		UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(
			this,
			TEXT("SlamJump"),
			JumpRot,
			Distance,
			JumpHeight,
			Duration,
			0.1f,
			true,
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,
			0.f,
			nullptr,
			nullptr);

	Jump->OnFinish.AddDynamic(this, &UEnemySlamAttack::OnJumpFinished);
	Jump->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* LandEvt = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		GYGameplayTags::Event_Enemy_Slam_Land,
		nullptr,
		false);
	LandEvt->EventReceived.AddDynamic(this, &UEnemySlamAttack::OnLandImpact);
	LandEvt->ReadyForActivation();
}

void UEnemySlamAttack::OnJumpFinished()
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return;

	if (UAnimInstance* Anim = Avatar->GetMesh()->GetAnimInstance())
	{
		Anim->Montage_JumpToSection(TEXT("Land"), AttackMontage);
	}
	ExecuteImpact();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEnemySlamAttack::OnLandImpact(FGameplayEventData Payload)
{
	ExecuteImpact();
}

void UEnemySlamAttack::ResolveTargetLocation(const FGameplayEventData* TriggerEventData)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		TargetLocation = FVector::ZeroVector;
		return;
	}

	const FVector Self = Avatar->GetActorLocation();

	if (TargetMode == ESlamTargetMode::FixedDistance)
	{
		FVector Forward = Avatar->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
		TargetLocation = Self + Forward * FixedDistance;
		return;
	}

	AActor* Target = nullptr;
	if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Avatar->GetController()))
	{
		Target = AI->GetTargetActor();
	}

	if (!Target)
	{
		FVector Forward = Avatar->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
		TargetLocation = Self + Forward * FixedDistance;
		return;
	}

	const FVector TargetPos = Target->GetActorLocation();
	FVector Dir = TargetPos - Self;
	Dir.Z = 0.f;
	const float Dist = Dir.Size();
	if (Dist < KINDA_SMALL_NUMBER)
	{
		TargetLocation = TargetPos;
		return;
	}
	Dir /= Dist;

	TargetLocation = TargetPos - Dir * TargetOffset;
}

void UEnemySlamAttack::ExecuteImpact()
{
	UWorld* World = GetWorld();
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!World || !Avatar) return;

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	FVector Origin = Avatar->GetActorLocation();
	if (USkeletalMeshComponent* Mesh = Avatar->GetMesh())
	{
		if (ImpactSocket != NAME_None && Mesh->DoesSocketExist(ImpactSocket))
		{
			Origin = Mesh->GetSocketLocation(ImpactSocket);
		}
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SlamImpact), false, Avatar);
	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ImpactRadius),
		Params);

	const FRichCurve* Curve = DamageFalloffCurve.GetRichCurveConst();

	TSet<UAbilitySystemComponent*> ProcessedASCs;
	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == Avatar) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC) continue;

		bool bAlreadyProcessed = false;
		ProcessedASCs.Add(TargetASC, &bAlreadyProcessed);
		if (bAlreadyProcessed) continue;

		const float Dist = FVector::Dist(Origin, HitActor->GetActorLocation());
		const float NormalizeDist = FMath::Clamp(Dist / FMath::Max(1.f, ImpactRadius), 0.f, 1.f);
		const float Falloff = Curve ? Curve->Eval(NormalizeDist) : 1.f;

		FGYHitContext HitContext;
		HitContext.SourceASC = OwnerASC;
		HitContext.TargetASC = TargetASC;
		HitContext.bGivesParriedReaction = false;

		if (HitDamageWeights.IsValidIndex(0))
		{
			const FHitDamageWeight& W = HitDamageWeights[0];
			HitContext.MotionMultiplier = W.Multiplicative * Falloff;
			HitContext.Additive = W.Additive;
			HitContext.StaggerAmount = W.Stagger;
			HitContext.StunAmount = W.Stun;
			HitContext.KnockbackStrength = W.KnockbackStrength;
		}
		UGYCombatStatics::ApplyHitImpact(HitContext);
	}

	if (LandCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = Origin;
		CueParams.SourceObject = Avatar;
		OwnerASC->ExecuteGameplayCue(LandCueTag, CueParams);
	}

}
