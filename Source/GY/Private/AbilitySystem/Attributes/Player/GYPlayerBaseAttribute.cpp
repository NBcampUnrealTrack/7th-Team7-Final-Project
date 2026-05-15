#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYPlayerBaseAttribute::UGYPlayerBaseAttribute()
{
	InitCurrentHealth(100.f);
	InitMaxHealth(100.f);
	InitAttack(10.f);
	InitDefense(5.f);
}

void UGYPlayerBaseAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UGYPlayerBaseAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
}
