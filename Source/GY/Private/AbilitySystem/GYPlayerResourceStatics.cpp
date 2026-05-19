#include "AbilitySystem/GYPlayerResourceStatics.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"

static void ApplyInstantGEToAttribute(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
{
	if (!ASC || Magnitude == 0.f) return;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;
	GE->Modifiers.SetNum(1);
	GE->Modifiers[0].ModifierMagnitude = FScalableFloat(Magnitude);
	GE->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
	GE->Modifiers[0].Attribute = Attribute;

	FGameplayEffectSpec Spec(GE, ASC->MakeEffectContext(), 1.f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYPlayerResourceStatics::ApplyFocusUse(UAbilitySystemComponent* ASC, float Amount)
{
	ApplyInstantGEToAttribute(ASC, UGYPlayerAttribute::GetCurrentFocusAttribute(), -Amount);
}

void UGYPlayerResourceStatics::ApplyFocusGain(UAbilitySystemComponent* ASC, float BaseAmount)
{
	if (!ASC) return;
	const UGYPlayerAttribute* PlayerAttr = ASC->GetSet<UGYPlayerAttribute>();
	const float RegenRate = PlayerAttr ? PlayerAttr->GetFocusRegenRate() : 1.f;
	ApplyInstantGEToAttribute(ASC, UGYPlayerAttribute::GetCurrentFocusAttribute(), BaseAmount * RegenRate);
}

void UGYPlayerResourceStatics::ApplyStaminaUse(UAbilitySystemComponent* ASC, float Amount)
{
	ApplyInstantGEToAttribute(ASC, UGYPlayerAttribute::GetCurrentStaminaAttribute(), -Amount);
}

void UGYPlayerResourceStatics::UseStamina(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYPlayerAttribute* PlayerAttr = ASC->GetSet<UGYPlayerAttribute>();
	if (!PlayerAttr || PlayerAttr->GetCurrentStamina() <= 0.f) return;

	ApplyStaminaUse(ASC, Amount);
	ASC->RescheduleStaminaRegen();
}

void UGYPlayerResourceStatics::ApplyHitResUse(UAbilitySystemComponent* ASC, float Amount)
{
	ApplyInstantGEToAttribute(ASC, UGYAdditionalAttribute::GetCurrentHitResAttribute(), -Amount);
}

void UGYPlayerResourceStatics::DecreaseHitRes(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentHitRes() <= 0.f) return;

	ApplyHitResUse(ASC, Amount);
	ASC->RescheduleHitResRegen();
}

void UGYPlayerResourceStatics::ApplyPoiseUse(UAbilitySystemComponent* ASC, float Amount)
{
	ApplyInstantGEToAttribute(ASC, UGYAdditionalAttribute::GetCurrentPoiseAttribute(), -Amount);
}

void UGYPlayerResourceStatics::DecreasePoise(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentPoise() <= 0.f) return;

	ApplyPoiseUse(ASC, Amount);
	ASC->ReschedulePoiseRegen();
}

void UGYPlayerResourceStatics::ApplyAttributeDelta(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
{
	ApplyInstantGEToAttribute(ASC, Attribute, Magnitude);
}
