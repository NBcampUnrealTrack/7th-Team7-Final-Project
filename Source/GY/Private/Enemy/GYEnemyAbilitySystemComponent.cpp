#include "Enemy/GYEnemyAbilitySystemComponent.h"

#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "AbilitySystem/GYRegenDelayEffect.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "Core/GameplayTags/StateTags.h"


void UGYEnemyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void UGYEnemyAbilitySystemComponent::HandleVitalAccumulation(const FGameplayAttribute& ChangedAttribute,
	float CurrentValue)
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyASC] HandleVitalAccumulation Attr=%s Curr=%.2f"),
		*ChangedAttribute.GetName(), CurrentValue);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

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
				ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}

void UGYEnemyAbilitySystemComponent::NotifyAttributeChanged(const FGameplayAttribute& Attribute)
{
	if (Attribute == UGYEnemyVitalAttributeSet::GetCurrentStaggerAttribute())
		RefreshRegenDelay(GYStateTags::State_Regen_Delay_Stagger, StaggerRegenDelayDuration);
	else if (Attribute == UGYVitalAttributeSet::GetCurrentStunAttribute())
		RefreshRegenDelay(GYStateTags::State_Regen_Delay_Stun, StunRegenDelayDuration);
}

void UGYEnemyAbilitySystemComponent::ApplyRegenEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	ApplyEffect(StaggerRegenEffect, StaggerRegenGEHandle);
	ApplyEffect(StunRegenEffect, StunRegenGEHandle);
	ResetRegenDelays();
}

void UGYEnemyAbilitySystemComponent::ApplyCombatTag()
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyASC] ApplyCombatTag on %s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("null"));
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
	UE_LOG(LogTemp, Warning, TEXT("[EnemyASC] RemoveCombatTag on %s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("null"));
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

void UGYEnemyAbilitySystemComponent::RefreshRegenDelay(const FGameplayTag& DelayTag, float Duration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat)) return;
	if (!RegenDelayEffect || Duration <= 0.f || !DelayTag.IsValid()) return;

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(RegenDelayEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetDuration(Duration, true);
	Spec.Data->DynamicGrantedTags.AddTag(DelayTag);

	ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UGYEnemyAbilitySystemComponent::ResetRegenDelays()
{
	FGameplayTagContainer DelayTags;
	DelayTags.AddTag(GYStateTags::State_Regen_Delay_Stagger);
	DelayTags.AddTag(GYStateTags::State_Regen_Delay_Stun);
	RemoveActiveEffectsWithGrantedTags(DelayTags);
}
