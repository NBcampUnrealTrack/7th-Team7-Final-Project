#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "Data/GYStatScalingData.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYCoreStatAttributeSet::UGYCoreStatAttributeSet()
{
	InitStrength(0.f);
	InitDexterity(0.f);
	InitEvasionInvincibilityTime(0.2f);
}

void UGYCoreStatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYCoreStatAttributeSet, Strength);
	DOREPLIFETIME(UGYCoreStatAttributeSet, Dexterity);
	DOREPLIFETIME(UGYCoreStatAttributeSet, EvasionInvincibilityTime);
}

void UGYCoreStatAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, Strength, OldStrength);
}

void UGYCoreStatAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldDexterity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, Dexterity, OldDexterity);
}

void UGYCoreStatAttributeSet::OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, EvasionInvincibilityTime, OldEvasionInvincibilityTime);
}

void UGYCoreStatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (!StatScalingData) return;

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const float Magnitude = Data.EvaluatedData.Magnitude;

	UGYVitalAttributeSet* Vital = const_cast<UGYVitalAttributeSet*>(ASC->GetSet<UGYVitalAttributeSet>());
	UGYDamageAttributeSet* Damage = const_cast<UGYDamageAttributeSet*>(ASC->GetSet<UGYDamageAttributeSet>());
	UGYWeaponAttribute* Weapon = const_cast<UGYWeaponAttribute*>(ASC->GetSet<UGYWeaponAttribute>());

	if (Data.EvaluatedData.Attribute == GetStrengthAttribute())
	{
		if (Vital)
		{
			const float OldMax = Vital->GetMaxHealth();
			const float NewMax = OldMax + Magnitude * StatScalingData->StrengthToMaxHealth;
			Vital->SetMaxHealth(NewMax);
			if (OldMax > 0.f)
			{
				Vital->SetCurrentHealth(FMath::Clamp(Vital->GetCurrentHealth() * NewMax / OldMax, 0.f, NewMax));
			}
		}
		RecalculateWeaponMultiplier(ASC, Weapon);
	}
	else if (Data.EvaluatedData.Attribute == GetDexterityAttribute())
	{
		if (Damage)
		{
			Damage->SetCriticalRate(Damage->GetCriticalRate() + Magnitude * StatScalingData->DexterityToCriticalRate);
		}
		SetEvasionInvincibilityTime(GetEvasionInvincibilityTime() + Magnitude * StatScalingData->DexterityToEvasionInvincibilityTime);
		RecalculateWeaponMultiplier(ASC, Weapon);
	}
}

void UGYCoreStatAttributeSet::RecalculateWeaponMultiplier(UAbilitySystemComponent* ASC, UGYWeaponAttribute* Weapon)
{
	if (!StatScalingData || !Weapon || !ASC) return;

	const FWeaponStyleScalingFactors* Factors = nullptr;
	for (const TPair<FGameplayTag, FWeaponStyleScalingFactors>& Pair : StatScalingData->WeaponStyleFactors)
	{
		if (ASC->HasMatchingGameplayTag(Pair.Key))
		{
			Factors = &Pair.Value;
			break;
		}
	}

	if (!Factors)
	{
		Weapon->SetWeaponDamageMultiplier(1.f);
		return;
	}

	Weapon->SetWeaponDamageMultiplier(1.f + GetStrength() * Factors->StrengthFactor + GetDexterity() * Factors->DexterityFactor);
}
