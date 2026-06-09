#include "AbilitySystem/GYPlayerResourceStatics.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

static void PlayerResource_ApplyInstantGE(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
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

void UGYPlayerResourceStatics::ApplyStaminaUse(UAbilitySystemComponent* ASC, float Amount)
{
	PlayerResource_ApplyInstantGE(ASC, UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(), -Amount);
}

void UGYPlayerResourceStatics::UseStamina(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYPlayerVitalAttributeSet* PlayerVital = ASC->GetSet<UGYPlayerVitalAttributeSet>();
	if (!PlayerVital || PlayerVital->GetCurrentStamina() <= 0.f) return;

	ApplyStaminaUse(ASC, Amount);
	ASC->NotifyAttributeChanged(UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute());
}

void UGYPlayerResourceStatics::ApplyAttributeDelta(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
{
	PlayerResource_ApplyInstantGE(ASC, Attribute, Magnitude);
}
