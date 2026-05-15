#include "AbilitySystem/Attributes/Enemy/GYEnemyAdditionalAttribute.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UGYEnemyAdditionalAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
}

void UGYEnemyAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentPoiseAttribute())
	{
		SetCurrentPoise(FMath::Clamp(GetCurrentPoise(), 0.f, GetMaxPoise()));
	}
}
