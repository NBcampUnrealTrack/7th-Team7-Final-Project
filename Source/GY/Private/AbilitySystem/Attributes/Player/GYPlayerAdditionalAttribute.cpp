#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerAdditionalAttribute::UGYPlayerAdditionalAttribute()
{
	InitHitResistance(5.f);
	InitCurrentPoise(100.f);
	InitMaxPoise(100.f);
	InitCriticalRate(0.05f);
	InitCriticalMultiplier(1.5f);
}

void UGYPlayerAdditionalAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
}

void UGYPlayerAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentPoiseAttribute())
	{
		SetCurrentPoise(FMath::Clamp(GetCurrentPoise(), 0.f, GetMaxPoise()));
	}
}
