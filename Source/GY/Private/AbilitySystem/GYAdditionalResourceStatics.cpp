#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "AbilitySystem/GYPlayerResourceStatics.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystemComponent.h"

static void AdditionalResource_ApplyInstantGE(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
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

void UGYAdditionalResourceStatics::ApplyStaggerUse(UAbilitySystemComponent* ASC, float Amount)
{
	AdditionalResource_ApplyInstantGE(ASC, UGYAdditionalAttribute::GetCurrentStaggerAttribute(), -Amount);
}

void UGYAdditionalResourceStatics::DecreaseStagger(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentStagger() <= 0.f) return;

	ApplyStaggerUse(ASC, Amount);
	ASC->NotifyAttributeDecreased(UGYAdditionalAttribute::GetCurrentStaggerAttribute());
}

void UGYAdditionalResourceStatics::ApplyStunUse(UAbilitySystemComponent* ASC, float Amount)
{
	AdditionalResource_ApplyInstantGE(ASC, UGYAdditionalAttribute::GetCurrentStunAttribute(), -Amount);
}

void UGYAdditionalResourceStatics::DecreaseStun(UGYAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentStun() <= 0.f) return;

	ApplyStunUse(ASC, Amount);
	ASC->NotifyAttributeDecreased(UGYAdditionalAttribute::GetCurrentStunAttribute());
}
