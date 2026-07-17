#include "Enemy/GYEnemyAbilitySystemComponent.h"

#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "AbilitySystem/GYRegenDelayEffect.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/Character.h"


void UGYEnemyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void UGYEnemyAbilitySystemComponent::HandleVitalAccumulation(const FGameplayAttribute& ChangedAttribute,
	float CurrentValue)
{

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (HasMatchingGameplayTag(GYStateTags::State_Combat_SuperArmor)) return;

	for (const FGYDisableThreshold& Threshold : DisableThresholds)
	{
		if (Threshold.CurrentAttribute != ChangedAttribute) continue;

		// 이미 발동 상태면 재발동하지 않는다 (latch).
		if (Threshold.StateTag.IsValid() && HasMatchingGameplayTag(Threshold.StateTag)) continue;

		const float MaxValue = Threshold.MaxAttribute.IsValid() ? GetNumericAttributeBase(Threshold.MaxAttribute) : 0.f;
		if (MaxValue <= 0.f || CurrentValue < MaxValue) continue;

		// 통 리셋
		SetNumericAttributeBase(Threshold.CurrentAttribute, 0.f);

		// 진행 중 공격 등 취소
		if (!Threshold.CancelAbilityTags.IsEmpty())
		{
			FGameplayTagContainer CancelTags = Threshold.CancelAbilityTags;
			CancelAbilities(&CancelTags);
		}

		// 지속형 GE 적용 → StateTag 부여 (Duration이 곧 CC 지속시간)
		if (IsValid(Threshold.DisableEffect))
		{
			FGameplayEffectContextHandle Context = MakeEffectContext();
			FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(Threshold.DisableEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				if (Threshold.Duration > 0.f)
				{
					Spec.Data->SetDuration(Threshold.Duration, true);
				}
				ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}


void UGYEnemyAbilitySystemComponent::ApplyRegenEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	ApplyEffect(StaggerRegenEffect, StaggerRegenGEHandle);
	ApplyEffect(StunRegenEffect, StunRegenGEHandle);
	ApplyEffect(ActivityPointsRegenEffect, ActivityPointsRegenEffectHandle);
}

void UGYEnemyAbilitySystemComponent::ResetForRespawn()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	RemoveActiveEffects(FGameplayEffectQuery());

	StaggerRegenGEHandle = FActiveGameplayEffectHandle();
	StunRegenGEHandle = FActiveGameplayEffectHandle();
	ActivityPointsRegenEffectHandle = FActiveGameplayEffectHandle();
	CombatStateEffectHandle = FActiveGameplayEffectHandle();

	ApplyRegenEffects();
}

void UGYEnemyAbilitySystemComponent::ApplyActivityPointsUsedEffect()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	FGameplayEffectSpec Spec(ActivityPointsUsedEffect->GetDefaultObject<UGameplayEffect>(), MakeEffectContext(), 1.f);
	ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYEnemyAbilitySystemComponent::ApplyCombatTag()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!CombatStateEffect) return;
	if (CombatStateEffectHandle.IsValid()) return;

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(CombatStateEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	CombatStateEffectHandle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UGYEnemyAbilitySystemComponent::RemoveCombatTag()
{
	if (CombatStateEffectHandle.IsValid())
	{
		RemoveActiveGameplayEffect(CombatStateEffectHandle, 1);
		CombatStateEffectHandle = FActiveGameplayEffectHandle();
	}
}

void UGYEnemyAbilitySystemComponent::ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass,
                                                 FActiveGameplayEffectHandle& Handle)
{
	if (!EffectClass || Handle.IsValid()) return;
	FGameplayEffectSpec Spec(EffectClass->GetDefaultObject<UGameplayEffect>(), MakeEffectContext(), 1.f);
	Handle = ApplyGameplayEffectSpecToSelf(Spec);
}
