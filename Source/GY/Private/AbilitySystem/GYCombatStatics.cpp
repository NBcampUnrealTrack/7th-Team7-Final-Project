#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Logging/GYLogManager.h"

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

void UGYCombatStatics::ApplyDamage(UAbilitySystemComponent* ASC, float RawDamage)
{
	if (!ASC) return;
	const UGYBaseAttribute* Base = ASC->GetSet<UGYBaseAttribute>();
	const float Defense = Base ? Base->GetDefense() : 0.f;
	const float Effective = FMath::Max(0.f, RawDamage - Defense);

	GY_WARN(Combat, ESK, "[CombatStatics] ApplyDamage - Raw: %.1f, Defense: %.1f, Effective: %.1f, Target: %s",
	RawDamage, Defense, Effective,
	ASC->GetAvatarActor() ? *ASC->GetAvatarActor()->GetName() : TEXT("Unknown"));

	ApplyInstantGEToAttribute(ASC, UGYBaseAttribute::GetCurrentHealthAttribute(), -Effective);
}

void UGYCombatStatics::ApplyHeal(UAbilitySystemComponent* ASC, float HealAmount)
{
	ApplyInstantGEToAttribute(ASC, UGYBaseAttribute::GetCurrentHealthAttribute(), HealAmount);
}

float UGYCombatStatics::GetCurrentHealth(const UAbilitySystemComponent* ASC)
{
	if (!ASC) return 0.f;
	const UGYBaseAttribute* Base = ASC->GetSet<UGYBaseAttribute>();
	return Base ? Base->GetCurrentHealth() : 0.f;
}

float UGYCombatStatics::GetMaxHealth(const UAbilitySystemComponent* ASC)
{
	if (!ASC) return 0.f;
	const UGYBaseAttribute* Base = ASC->GetSet<UGYBaseAttribute>();
	return Base ? Base->GetMaxHealth() : 0.f;
}

bool UGYCombatStatics::IsAlive(const UAbilitySystemComponent* ASC)
{
	return GetCurrentHealth(ASC) > 0.f;
}
