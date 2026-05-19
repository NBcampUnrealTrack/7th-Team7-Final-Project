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

	ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
	ScheduleEffect(HitResRegenEffect, HitResRegenGEHandle, HitResRegenDelayHandle, &UGYAbilitySystemComponent::StartHitResRegen);
	ScheduleEffect(PoiseRegenEffect, PoiseRegenGEHandle, PoiseRegenDelayHandle, &UGYAbilitySystemComponent::StartPoiseRegen);
}

void UGYAbilitySystemComponent::RescheduleStaminaRegen()
{
	ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
}
void UGYAbilitySystemComponent::RescheduleHitResRegen()
{
	ScheduleEffect(HitResRegenEffect, HitResRegenGEHandle, HitResRegenDelayHandle, &UGYAbilitySystemComponent::StartHitResRegen);
}
void UGYAbilitySystemComponent::ReschedulePoiseRegen()
{
	ScheduleEffect(PoiseRegenEffect, PoiseRegenGEHandle, PoiseRegenDelayHandle, &UGYAbilitySystemComponent::StartPoiseRegen);
}

void UGYAbilitySystemComponent::OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (StaminaRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StaminaRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
	if (HitResRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(HitResRegenEffect)->CombatTag == Tag)
		ScheduleEffect(HitResRegenEffect, HitResRegenGEHandle, HitResRegenDelayHandle, &UGYAbilitySystemComponent::StartHitResRegen);
	if (PoiseRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(PoiseRegenEffect)->CombatTag == Tag)
		ScheduleEffect(PoiseRegenEffect, PoiseRegenGEHandle, PoiseRegenDelayHandle, &UGYAbilitySystemComponent::StartPoiseRegen);
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
void UGYAbilitySystemComponent::StartHitResRegen()
{
	if (HitResRegenGEHandle.IsValid()) return;
	const UGYAdditionalAttribute* AdditionalAttr = GetSet<UGYAdditionalAttribute>();
	if (AdditionalAttr && GetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentHitResAttribute()) >= AdditionalAttr->GetMaxHitRes()) return;
	ApplyEffect(HitResRegenEffect, HitResRegenGEHandle);
}
void UGYAbilitySystemComponent::StartPoiseRegen()
{
	if (PoiseRegenGEHandle.IsValid()) return;
	const UGYAdditionalAttribute* AdditionalAttr = GetSet<UGYAdditionalAttribute>();
	if (AdditionalAttr && GetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentPoiseAttribute()) >= AdditionalAttr->GetMaxPoise()) return;
	ApplyEffect(PoiseRegenEffect, PoiseRegenGEHandle);
}
