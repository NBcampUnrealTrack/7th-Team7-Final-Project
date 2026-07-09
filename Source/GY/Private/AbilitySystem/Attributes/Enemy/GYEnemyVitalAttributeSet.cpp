#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

void UGYEnemyVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGYEnemyVitalAttributeSet, ActivityPoints);
	DOREPLIFETIME(UGYEnemyVitalAttributeSet, MaxActivityPoints);

}

void UGYEnemyVitalAttributeSet::OnRep_ActivityPoints(const FGameplayAttributeData& OldActivityPoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYEnemyVitalAttributeSet, ActivityPoints, OldActivityPoints);

}

void UGYEnemyVitalAttributeSet::OnRep_MaxActivityPoints(const FGameplayAttributeData& OldMaxActivityPoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYEnemyVitalAttributeSet, MaxActivityPoints, OldMaxActivityPoints);
}


void UGYEnemyVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	if (Attribute == GetActivityPointsAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxActivityPoints());
	}

}

void UGYEnemyVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);


	UGYEnemyAbilitySystemComponent* EnemyASC = Cast<UGYEnemyAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (!EnemyASC) return;

	if (Data.EvaluatedData.Attribute == GetActivityPointsAttribute())
	{
		SetActivityPoints(FMath::Clamp(GetActivityPoints(), 0.f, GetMaxActivityPoints()));

		if (Data.EvaluatedData.Magnitude < 0.f)
		{
			EnemyASC->ApplyActivityPointsUsedEffect();
		}
		return;
	}

	float CurrentValue = 0.f;
	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
		CurrentValue = GetCurrentStagger();
	else if (Data.EvaluatedData.Attribute == GetCurrentStunAttribute())
		CurrentValue = GetCurrentStun();
	else
		return;

	EnemyASC->HandleVitalAccumulation(Data.EvaluatedData.Attribute, CurrentValue);
}

void UGYEnemyVitalAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data, float DamageAmount)
{
	Super::HandleIncomingDamage(Data, DamageAmount);

	UAbilitySystemComponent* SourceASC =
		Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
	UGYCombatStatics::ReportDamageToPerception(&Data.Target, SourceASC, DamageAmount);
}
