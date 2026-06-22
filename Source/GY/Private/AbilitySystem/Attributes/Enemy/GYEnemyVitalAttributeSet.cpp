#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"

void UGYEnemyVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UGYEnemyVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	float CurrentValue = 0.f;
	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
		CurrentValue = GetCurrentStagger();
	else if (Data.EvaluatedData.Attribute == GetCurrentStunAttribute())
		CurrentValue = GetCurrentStun();
	else
		return;

	if (UGYEnemyAbilitySystemComponent* EnemyASC = Cast<UGYEnemyAbilitySystemComponent>(
		GetOwningAbilitySystemComponent()))
	{
		EnemyASC->HandleVitalAccumulation(Data.EvaluatedData.Attribute, CurrentValue);
	}
}

void UGYEnemyVitalAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data, float DamageAmount)
{
	Super::HandleIncomingDamage(Data, DamageAmount);

	UAbilitySystemComponent* SourceASC =
		Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
	UGYCombatStatics::ReportDamageToPerception(&Data.Target, SourceASC, DamageAmount);
}
