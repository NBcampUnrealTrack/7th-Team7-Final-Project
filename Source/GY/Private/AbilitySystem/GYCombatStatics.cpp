#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/FactionTags.h"
#include "GameplayEffect.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "Logging/GYLogManager.h"

static bool IsSameFaction(UAbilitySystemComponent* A, UAbilitySystemComponent* B)
{
	if (!A || !B) return false;

	const bool AEnemy  = A->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy);
	const bool BEnemy  = B->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy);
	const bool APlayer = A->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Player);
	const bool BPlayer = B->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Player);

	return (AEnemy && BEnemy) || (APlayer && BPlayer);
}

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

void UGYCombatStatics::ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage)
{
	if (!TargetASC || RawDamage <= 0.f) return;

	ApplyInstantGEToAttribute(TargetASC, UGYBaseAttribute::GetCurrentHealthAttribute(), -RawDamage);

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
	{
		UGYAdditionalResourceStatics::DecreaseStagger(GYASC, RawDamage);
		UGYAdditionalResourceStatics::DecreaseStun(GYASC, RawDamage);
	}
}

void UGYCombatStatics::ApplyDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC)
{
	if (!TargetASC) return;

	if (SourceASC && IsSameFaction(SourceASC, TargetASC))
	{
		return;
	}

	const UGYBaseAttribute* Base = TargetASC->GetSet<UGYBaseAttribute>();
	const float Defense = Base ? Base->GetDefense() : 0.f;
	const float Effective = FMath::Max(0.f, RawDamage - Defense);

	ApplyInstantGEToAttribute(TargetASC, UGYBaseAttribute::GetCurrentHealthAttribute(), -Effective);

	if (Effective > 0.f)
	{
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
		{
			UGYAdditionalResourceStatics::DecreaseStagger(GYASC, Effective);
			UGYAdditionalResourceStatics::DecreaseStun(GYASC, Effective);
		}
	}
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
