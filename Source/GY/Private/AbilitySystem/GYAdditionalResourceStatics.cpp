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
	AdditionalResource_ApplyInstantGE(ASC, UGYAdditionalAttribute::GetCurrentStaggerAttribute(), Amount);
}

void UGYAdditionalResourceStatics::IncreaseStagger(UAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentStagger() >= AdditionalAttr->GetMaxStagger()) return;

	ApplyStaggerUse(ASC, Amount);
}

void UGYAdditionalResourceStatics::ApplyStunUse(UAbilitySystemComponent* ASC, float Amount)
{
	AdditionalResource_ApplyInstantGE(ASC, UGYAdditionalAttribute::GetCurrentStunAttribute(), Amount);
}

void UGYAdditionalResourceStatics::IncreaseStun(UAbilitySystemComponent* ASC, float Amount)
{
	if (!ASC) return;
	const UGYAdditionalAttribute* AdditionalAttr = ASC->GetSet<UGYAdditionalAttribute>();
	if (!AdditionalAttr || AdditionalAttr->GetCurrentStun() >= AdditionalAttr->GetMaxStun()) return;

	ApplyStunUse(ASC, Amount);
}

void UGYAdditionalResourceStatics::ApplyAttributeDelta(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Amount)
{
	if (!ASC || !Attribute.IsValid() || Amount == 0.f) return;
	AdditionalResource_ApplyInstantGE(ASC, Attribute, Amount);
	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
		GYASC->NotifyAttributeChanged(Attribute);
}
