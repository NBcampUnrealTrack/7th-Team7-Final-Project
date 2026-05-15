#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Data/GYStatScalingData.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerAttribute::UGYPlayerAttribute()
{
	InitCurrentStamina(100.f);
	InitMaxStamina(100.f);
	InitCurrentFocus(50.f);
	InitMaxFocus(50.f);
	InitStrength(5.f);
	InitDexterity(5.f);
	InitIntelligence(5.f);
	InitEvasionInvincibilityTime(0.2f);
	InitFocusRegenRate(1.f);
}

void UGYPlayerAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYPlayerAttribute, CurrentStamina);
	DOREPLIFETIME(UGYPlayerAttribute, MaxStamina);
	DOREPLIFETIME(UGYPlayerAttribute, CurrentFocus);
	DOREPLIFETIME(UGYPlayerAttribute, MaxFocus);
	DOREPLIFETIME(UGYPlayerAttribute, Strength);
	DOREPLIFETIME(UGYPlayerAttribute, Dexterity);
	DOREPLIFETIME(UGYPlayerAttribute, Intelligence);
	DOREPLIFETIME(UGYPlayerAttribute, EvasionInvincibilityTime);
	DOREPLIFETIME(UGYPlayerAttribute, FocusRegenRate);
}

void UGYPlayerAttribute::OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, CurrentStamina, OldCurrentStamina);
}

void UGYPlayerAttribute::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, MaxStamina, OldMaxStamina);
}

void UGYPlayerAttribute::OnRep_CurrentFocus(const FGameplayAttributeData& OldCurrentFocus)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, CurrentFocus, OldCurrentFocus);
}

void UGYPlayerAttribute::OnRep_MaxFocus(const FGameplayAttributeData& OldMaxFocus)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, MaxFocus, OldMaxFocus);
}

void UGYPlayerAttribute::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Strength, OldStrength);
}

void UGYPlayerAttribute::OnRep_Dexterity(const FGameplayAttributeData& OldDexterity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Dexterity, OldDexterity);
}

void UGYPlayerAttribute::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, Intelligence, OldIntelligence);
}

void UGYPlayerAttribute::OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, EvasionInvincibilityTime, OldEvasionInvincibilityTime);
}

void UGYPlayerAttribute::OnRep_FocusRegenRate(const FGameplayAttributeData& OldFocusRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerAttribute, FocusRegenRate, OldFocusRegenRate);
}

void UGYPlayerAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetCurrentFocusAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxFocus());
	}
}

void UGYPlayerAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentStaminaAttribute())
	{
		SetCurrentStamina(FMath::Clamp(GetCurrentStamina(), 0.f, GetMaxStamina()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetCurrentFocusAttribute())
	{
		SetCurrentFocus(FMath::Clamp(GetCurrentFocus(), 0.f, GetMaxFocus()));
		return;
	}

	if (!StatScalingData) return;

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const float Magnitude = Data.EvaluatedData.Magnitude;

	UGYPlayerBaseAttribute* Base = const_cast<UGYPlayerBaseAttribute*>(ASC->GetSet<UGYPlayerBaseAttribute>());
	UGYPlayerAdditionalAttribute* Additional = const_cast<UGYPlayerAdditionalAttribute*>(ASC->GetSet<UGYPlayerAdditionalAttribute>());

	if (Data.EvaluatedData.Attribute == GetStrengthAttribute())
	{
		if (Base)
		{
			Base->SetMaxHealth(Base->GetMaxHealth() + Magnitude * StatScalingData->StrengthToMaxHealth);
		}
		if (Additional)
		{
			Additional->SetHitResistance(Additional->GetHitResistance() + Magnitude * StatScalingData->StrengthToHitResistance);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetDexterityAttribute())
	{
		if (Additional)
		{
			Additional->SetCriticalRate(Additional->GetCriticalRate() + Magnitude * StatScalingData->DexterityToCriticalRate);
		}
		SetEvasionInvincibilityTime(GetEvasionInvincibilityTime() + Magnitude * StatScalingData->DexterityToEvasionInvincibilityTime);
	}
	else if (Data.EvaluatedData.Attribute == GetIntelligenceAttribute())
	{
		SetMaxFocus(GetMaxFocus() + Magnitude * StatScalingData->IntelligenceToMaxFocus);
		SetFocusRegenRate(GetFocusRegenRate() + Magnitude * StatScalingData->IntelligenceToFocusRegenRate);
	}
}
