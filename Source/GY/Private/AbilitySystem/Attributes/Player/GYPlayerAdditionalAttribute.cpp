#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerAdditionalAttribute::UGYPlayerAdditionalAttribute()
{
	InitCriticalRate(0.05f);
	InitCriticalMultiplier(1.5f);
}

void UGYPlayerAdditionalAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UGYPlayerAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}
