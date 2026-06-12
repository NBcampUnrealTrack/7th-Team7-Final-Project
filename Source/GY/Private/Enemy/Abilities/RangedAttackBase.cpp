#include "Enemy/Abilities/RangedAttackBase.h"
#include "Enemy/Abilities/RangedAttackBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Projectile/ProjectileBase.h"

void URangedAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilityTask_WaitGameplayEvent* LaunchTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_LaunchProjectile,
			nullptr,
			false);
	LaunchTask->EventReceived.AddDynamic(this, &URangedAttackBase::OnLaunchEvent);
	LaunchTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			nullptr,
			false);
	HitTask->EventReceived.AddDynamic(this, &URangedAttackBase::OnProjectileHit);
	HitTask->ReadyForActivation();

	PlayAttackMontage();
}

void URangedAttackBase::OnLaunchEvent(FGameplayEventData Payload)
{
	SpawnProjectile();
}

void URangedAttackBase::OnProjectileHit(FGameplayEventData Payload)
{
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get());
	if (!HitActor || !Instigator) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FHitResult HitResult;
	if (Payload.TargetData.IsValid(0))
	{
		const FGameplayAbilityTargetData* Data = Payload.TargetData.Get(0);
		if (const FHitResult* Found = Data->GetHitResult())
			HitResult = *Found;
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	// HP 데미지·DEF·크리 계산은 GE_HitImpact의 execution이 담당. 여기선 공격별 값만 컨텍스트로 전달.
	float MotionMultiplier = 1.f;
	float Additive = 0.f;
	if (HitDamageWeights.IsValidIndex(0))
	{
		const FHitDamageWeight& W = HitDamageWeights[0];
		MotionMultiplier = W.Multiplicative;
		Additive = W.Additive;
	}

	// 발사체 분산 시 데미지를 N분의 1로 — 배율에 흡수
	if (bDistributeDamage && ProjectileCount > 1)
	{
		MotionMultiplier /= static_cast<float>(ProjectileCount);
	}

	FGYHitContext HitContext;
	HitContext.SourceASC = OwnerASC;
	HitContext.TargetASC = TargetASC;
	HitContext.MotionMultiplier = MotionMultiplier;
	HitContext.Additive = Additive;
	// TODO: 적 공격별 경직/무력 값을 FHitDamageWeight에 추가해 전달. 지금은 예시용 임시 상수.
	HitContext.StaggerAmount = 25.f;
	HitContext.StunAmount = 10.f;

	UGYCombatStatics::ApplyHitImpact(HitContext);

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Normal = HitResult.ImpactNormal;
		CueParameters.Location = HitResult.ImpactPoint;
		CueParameters.SourceObject = Instigator;

		TargetASC->ExecuteGameplayCue(HitCueTag, CueParameters);
	}
}

void URangedAttackBase::SpawnProjectile()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !ProjectileClass) return;

	USkeletalMeshComponent* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh) return;

	const FVector LaunchPos = Mesh->GetSocketLocation(LaunchSocket);

	AActor* Target = nullptr;
	if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
		}
	}

	const FVector BaseDir = Target
	? (Target->GetActorLocation() - LaunchPos).GetSafeNormal()
	: Pawn->GetActorForwardVector();

	const int32 ShotCount = FMath::Max(1, ProjectileCount);

	for (int32 i = 0; i < ShotCount; ++i)
	{
		const float Ratio = (ShotCount == 1)
			? 0.f
			: (static_cast<float>(i) / static_cast<float>(ShotCount - 1)) - 0.5f;

		const float YawOffset = Ratio * SpreadAngle;
		const FRotator OffsetRot(0.f, YawOffset, 0.f);
		const FVector ShotDir = OffsetRot.RotateVector(BaseDir);

		FActorSpawnParameters Params;
		Params.Owner = Pawn;
		Params.Instigator = Pawn;

		AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(
			ProjectileClass, LaunchPos, ShotDir.Rotation(), Params);

		if (Projectile)
			Projectile->Launch(Pawn, ShotDir, ProjectileSpeed);
	}
}
