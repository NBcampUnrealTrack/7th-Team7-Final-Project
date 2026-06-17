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



	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		const float Delta = Data.EvaluatedData.Magnitude;

		if (Delta < 0.f)
		{
			UAbilitySystemComponent* SourceASC =
				Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
			UGYCombatStatics::ReportDamageToPerception(&Data.Target, SourceASC, -Delta);
		}
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
		return;
	}

	float CurrentValue = 0.f;
	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
		CurrentValue = GetCurrentStagger();
	else if (Data.EvaluatedData.Attribute == GetCurrentStunAttribute())
		CurrentValue = GetCurrentStun();
	else
		return;

	if (UGYEnemyAbilitySystemComponent* EnemyASC =
		Cast<UGYEnemyAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
	{
		EnemyASC->HandleVitalAccumulation(Data.EvaluatedData.Attribute, CurrentValue);
		if (Data.EvaluatedData.Magnitude > 0.f)
		{
			EnemyASC->NotifyAttributeChanged(Data.EvaluatedData.Attribute);
		}
	}
}
