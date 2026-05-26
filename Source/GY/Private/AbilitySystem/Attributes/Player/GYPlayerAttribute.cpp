#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Data/GYStatScalingData.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYWeaponAttribute.h"
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

	UGYPlayerBaseAttribute* Base = const_cast<UGYPlayerBaseAttribute*>(ASC->GetSet<UGYPlayerBaseAttribute>());
	UGYPlayerAdditionalAttribute* Additional = const_cast<UGYPlayerAdditionalAttribute*>(ASC->GetSet<UGYPlayerAdditionalAttribute>());
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
		if (Base)
		{
			Base->SetCurrentHealth(Base->GetMaxHealth());
		}
		if (Additional)
		{
			Additional->SetCurrentStagger(Additional->GetMaxStagger());
			Additional->SetCurrentStun(Additional->GetMaxStun());
		}
		SetCurrentStamina(GetMaxStamina());
		return;
	}


	if (Data.EvaluatedData.Attribute == GetStrengthAttribute())
	{
		if (Base)
		{
			const float OldMax = Base->GetMaxHealth();
			const float NewMax = OldMax + Magnitude * StatScalingData->StrengthToMaxHealth;
			Base->SetMaxHealth(NewMax);
			if (OldMax > 0.f)
			{
				Base->SetCurrentHealth(FMath::Clamp(Base->GetCurrentHealth() * NewMax / OldMax, 0.f, NewMax));
			}
		}
		if (Weapon)
		{
			Weapon->SetSwordAndShieldMultiplier(Weapon->GetSwordAndShieldMultiplier() + Magnitude * StatScalingData->StrengthToSwordAndShieldMultiplier);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetDexterityAttribute())
	{
		if (Additional)
		{
			Additional->SetCriticalRate(Additional->GetCriticalRate() + Magnitude * StatScalingData->DexterityToCriticalRate);
		}
		SetEvasionInvincibilityTime(GetEvasionInvincibilityTime() + Magnitude * StatScalingData->DexterityToEvasionInvincibilityTime);
		if (Weapon)
		{
			Weapon->SetSwordAndShieldMultiplier(Weapon->GetSwordAndShieldMultiplier() + Magnitude * StatScalingData->DexterityToSwordAndShieldMultiplier);
		}
	}
}
