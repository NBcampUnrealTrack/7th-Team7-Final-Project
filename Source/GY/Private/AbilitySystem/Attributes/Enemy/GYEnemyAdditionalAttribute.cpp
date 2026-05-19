#include "AbilitySystem/Attributes/Enemy/GYEnemyAdditionalAttribute.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UGYEnemyAdditionalAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UGYEnemyAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}
