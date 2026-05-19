#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"

void UGYAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (StaminaRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StaminaRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}
	if (StaggerRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StaggerRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}
	if (StunRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StunRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}

	ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
	ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
	ScheduleEffect(StunRegenEffect, StunRegenGEHandle, StunRegenDelayHandle, &UGYAbilitySystemComponent::StartStunRegen);
}

void UGYAbilitySystemComponent::RescheduleStaminaRegen()
{
	ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
}
void UGYAbilitySystemComponent::RescheduleStaggerRegen()
{
	ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
}
void UGYAbilitySystemComponent::RescheduleStunRegen()
{
	ScheduleEffect(StunRegenEffect, StunRegenGEHandle, StunRegenDelayHandle, &UGYAbilitySystemComponent::StartStunRegen);
}

void UGYAbilitySystemComponent::OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (StaminaRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StaminaRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
	if (StaggerRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StaggerRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
	if (StunRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StunRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StunRegenEffect, StunRegenGEHandle, StunRegenDelayHandle, &UGYAbilitySystemComponent::StartStunRegen);
}

void UGYAbilitySystemComponent::ScheduleEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle, void(UGYAbilitySystemComponent::* StartFunc)())
{
	StopEffect(Handle, DelayHandle);
	if (!EffectClass) return;

	const UGYPeriodicAttributeEffect* CDO = GetDefault<UGYPeriodicAttributeEffect>(EffectClass);
	const bool bHasTag = CDO->CombatTag.IsValid() && HasMatchingGameplayTag(CDO->CombatTag);
	const float Delay = bHasTag ? CDO->CombatStartDelay : 0.f;

	if (Delay > 0.f)
		GetWorld()->GetTimerManager().SetTimer(DelayHandle, this, StartFunc, Delay, false);
	else
		(this->*StartFunc)();
}

void UGYAbilitySystemComponent::ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle)
{
	if (!EffectClass || Handle.IsValid()) return;
	FGameplayEffectSpec Spec(EffectClass->GetDefaultObject<UGameplayEffect>(), MakeEffectContext(), 1.f);
	Handle = ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYAbilitySystemComponent::StopEffect(FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle)
{
	GetWorld()->GetTimerManager().ClearTimer(DelayHandle);
	if (Handle.IsValid())
	{
		RemoveActiveGameplayEffect(Handle);
		Handle.Invalidate();
	}
}

void UGYAbilitySystemComponent::StartStaminaRegen()
{
	if (StaminaRegenGEHandle.IsValid()) return;
	const UGYPlayerAttribute* PlayerAttr = GetSet<UGYPlayerAttribute>();
	if (PlayerAttr && GetNumericAttributeBase(UGYPlayerAttribute::GetCurrentStaminaAttribute()) >= PlayerAttr->GetMaxStamina()) return;
	ApplyEffect(StaminaRegenEffect, StaminaRegenGEHandle);
}
void UGYAbilitySystemComponent::StartStaggerRegen()
{
	if (StaggerRegenGEHandle.IsValid()) return;
	const UGYAdditionalAttribute* AdditionalAttr = GetSet<UGYAdditionalAttribute>();
	if (AdditionalAttr && GetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentStaggerAttribute()) >= AdditionalAttr->GetMaxStagger()) return;
	ApplyEffect(StaggerRegenEffect, StaggerRegenGEHandle);
}
void UGYAbilitySystemComponent::StartStunRegen()
{
	if (StunRegenGEHandle.IsValid()) return;
	const UGYAdditionalAttribute* AdditionalAttr = GetSet<UGYAdditionalAttribute>();
	if (AdditionalAttr && GetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentStunAttribute()) >= AdditionalAttr->GetMaxStun()) return;
	ApplyEffect(StunRegenEffect, StunRegenGEHandle);
}
