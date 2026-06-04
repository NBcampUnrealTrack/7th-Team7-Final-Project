#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AttackLogic/Block/GYBlockFragment.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"
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

void UGYCombatStatics::ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage)
{
	ApplyInstantGEToAttribute(TargetASC, UGYBaseAttribute::GetCurrentHealthAttribute(), -RawDamage);
}

void UGYCombatStatics::ApplyDamage(UAbilitySystemComponent* TargetASC, float RawDamage,
	FGameplayTag DodgeStateTag,
	FGameplayTagContainer ParryStateTags,
	FGameplayTag BlockStateTag)
{
	if (!TargetASC) return;

	if (UGYAbilitySystemComponent* TgtGY = Cast<UGYAbilitySystemComponent>(TargetASC))
		TgtGY->ApplyCombatTag();

	if (DodgeStateTag.IsValid() && TargetASC->HasMatchingGameplayTag(DodgeStateTag))
		return;

	if (!ParryStateTags.IsEmpty() && TargetASC->HasAnyMatchingGameplayTags(ParryStateTags))
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Parry_Hit;
		TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Parry_Hit, &Payload);
		return;
	}

	float DamageMultiplier = 1.f;
	if (BlockStateTag.IsValid() && TargetASC->HasMatchingGameplayTag(BlockStateTag))
	{
		FGameplayTagContainer OwnedTags;
		TargetASC->GetOwnedGameplayTags(OwnedTags);

		bool bFound = false;
		for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
		{
			if (bFound) break;
			for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
			{
				UGYPlayerGameplayAbility* GA = Cast<UGYPlayerGameplayAbility>(Instance);
				if (!GA || !GA->IsActive()) continue;
				if (!GA->GetAssetTags().HasTag(GYGameplayTags::Ability_Fragment_Block)) continue;
				if (UGYBlockFragment* BF = GA->GetFragment<UGYBlockFragment>())
				{
					if (const FGYBlockData* Data = BF->GetBestMatchingData(OwnedTags))
					{
						DamageMultiplier = Data->DamageReductionMultiplier;
					}
				}
				bFound = true;
				break;
			}
		}
	}

	const UGYBaseAttribute* Base = TargetASC->GetSet<UGYBaseAttribute>();
	const float Defense = Base ? Base->GetDefense() : 0.f;
	const float Effective = FMath::Max(0.f, (RawDamage * DamageMultiplier) - Defense);

	ApplyInstantGEToAttribute(TargetASC, UGYBaseAttribute::GetCurrentHealthAttribute(), -Effective);
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
