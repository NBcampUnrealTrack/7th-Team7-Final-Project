#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Data/GYStatScalingData.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerAttribute::UGYPlayerAttribute()
{
	InitCurrentStamina(100.f);
	InitMaxStamina(100.f);
	InitStrength(0.f);
	InitDexterity(0.f);
	InitEvasionInvincibilityTime(0.2f);
	InitLevel(1.f);
	InitSkillPoint(0.f);
	InitXP(0.f);
}

void UGYPlayerAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYPlayerAttribute, CurrentStamina);
	DOREPLIFETIME(UGYPlayerAttribute, MaxStamina);
	DOREPLIFETIME(UGYPlayerAttribute, Strength);
	DOREPLIFETIME(UGYPlayerAttribute, Dexterity);
	DOREPLIFETIME(UGYPlayerAttribute, EvasionInvincibilityTime);
	DOREPLIFETIME(UGYPlayerAttribute, Level);
	DOREPLIFETIME(UGYPlayerAttribute, SkillPoint);
	DOREPLIFETIME(UGYPlayerAttribute, XP);
}

void UGYPlayerAttribute::OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, CurrentStamina, OldCurrentStamina);
}

void UGYPlayerAttribute::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, MaxStamina, OldMaxStamina);
}

void UGYPlayerAttribute::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Strength, OldStrength);
}

void UGYPlayerAttribute::OnRep_Dexterity(const FGameplayAttributeData& OldDexterity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Dexterity, OldDexterity);
}

void UGYPlayerAttribute::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Level, OldLevel);
}

void UGYPlayerAttribute::OnRep_SkillPoint(const FGameplayAttributeData& OldSkillPoint)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, SkillPoint, OldSkillPoint);
}

void UGYPlayerAttribute::OnRep_XP(const FGameplayAttributeData& OldXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, XP, OldXP);
}

void UGYPlayerAttribute::OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, EvasionInvincibilityTime, OldEvasionInvincibilityTime);
}

void UGYPlayerAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, -GetMaxStamina(), GetMaxStamina());
	}
	if (Attribute == GetXPAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetLevelAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetSkillPointAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UGYPlayerAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (!StatScalingData) return;

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const float Magnitude = Data.EvaluatedData.Magnitude;

	UGYVitalAttributeSet* Vital = const_cast<UGYVitalAttributeSet*>(ASC->GetSet<UGYVitalAttributeSet>());
	UGYDamageAttributeSet* Damage = const_cast<UGYDamageAttributeSet*>(ASC->GetSet<UGYDamageAttributeSet>());
	UGYWeaponAttribute* Weapon = const_cast<UGYWeaponAttribute*>(ASC->GetSet<UGYWeaponAttribute>());

	if (Data.EvaluatedData.Attribute == GetCurrentStaminaAttribute())
	{
		SetCurrentStamina(FMath::Clamp(GetCurrentStamina(), -GetMaxStamina(), GetMaxStamina()));
		return;
	}


	if (Data.EvaluatedData.Attribute == GetXPAttribute())
	{
		if (NextLevelXPCurve && LevelUpEffect)
		{
			float MinTime, MaxTime;
			NextLevelXPCurve->GetTimeRange(MinTime, MaxTime);

			while (GetLevel() <= MaxTime && GetXP() >= NextLevelXPCurve->GetFloatValue(GetLevel()))
			{
				const float Threshold = NextLevelXPCurve->GetFloatValue(GetLevel());
				SetXP(GetXP() - Threshold);

				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(LevelUpEffect, GetLevel(), Context);
				if (SpecHandle.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
				}
			}
		}
		return;
	}

	if (Data.EvaluatedData.Attribute == GetLevelAttribute())
	{
		// 레벨업시 전부 회복
		if (Vital)
		{
			Vital->SetCurrentHealth(Vital->GetMaxHealth());
			Vital->SetCurrentStagger(Vital->GetMaxStagger());
			Vital->SetCurrentStun(Vital->GetMaxStun());
		}
		SetCurrentStamina(GetMaxStamina());
		return;
	}


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

void UGYPlayerAttribute::RecalculateWeaponMultiplier(UAbilitySystemComponent* ASC, UGYWeaponAttribute* Weapon)
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
