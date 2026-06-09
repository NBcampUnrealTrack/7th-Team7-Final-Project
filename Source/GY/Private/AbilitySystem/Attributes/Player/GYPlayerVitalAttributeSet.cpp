#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerVitalAttributeSet::UGYPlayerVitalAttributeSet()
{
	InitCurrentHealth(100.f);
	InitMaxHealth(100.f);
}

void UGYPlayerVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UGYPlayerVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
}
