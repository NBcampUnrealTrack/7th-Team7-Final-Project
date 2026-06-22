#include "Enemy/Abilities/EnemyMeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "Core/GameplayTags/EventTags.h"

void UEnemyMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	HitCount = 0;

	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			nullptr,
			false);
	HitTask->EventReceived.AddDynamic(this, &UEnemyMeleeAttack::OnWeaponHit);
	HitTask->ReadyForActivation();

	PlayAttackMontage();
}

void UEnemyMeleeAttack::OnWeaponHit(FGameplayEventData Payload)
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
		{
			HitResult = *Found;
		}
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	// HP 데미지·DEF·크리 계산은 GE_HitImpact의 execution이 담당. 여기선 공격별 값만 컨텍스트로 전달.
	FGYHitContext HitContext;
	HitContext.SourceASC = OwnerASC;
	HitContext.TargetASC = TargetASC;

	if (Payload.ContextHandle.IsValid())
	{
		const FParriedEventContext* CustomContext = StaticCast<const FParriedEventContext*>(Payload.ContextHandle.Get());
		if (CustomContext)
		{
			HitContext.SourceHitBone = CustomContext->SourceHitBone;
		}
	}

	if (HitDamageWeights.IsValidIndex(HitCount))
	{
		const FHitDamageWeight& W = HitDamageWeights[HitCount];
		HitContext.MotionMultiplier = W.Multiplicative;
		HitContext.Additive = W.Additive;
		HitContext.StaggerAmount = W.Stagger;
		HitContext.StunAmount = W.Stun;
	}

	UGYCombatStatics::ApplyHitImpact(HitContext);

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Normal = HitResult.ImpactNormal;
		CueParams.Location = HitResult.ImpactPoint;
		CueParams.SourceObject = Instigator;

		TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}
}
