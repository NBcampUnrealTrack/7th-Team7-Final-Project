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
	DOREPLIFETIME(UGYEnemyVitalAttributeSet, MovementSpeed);

}

void UGYEnemyVitalAttributeSet::OnRep_ActivityPoints(const FGameplayAttributeData& OldActivityPoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYEnemyVitalAttributeSet, ActivityPoints, OldActivityPoints);

}

void UGYEnemyVitalAttributeSet::OnRep_MaxActivityPoints(const FGameplayAttributeData& OldMaxActivityPoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYEnemyVitalAttributeSet, MaxActivityPoints, OldMaxActivityPoints);
}

void UGYEnemyVitalAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYEnemyVitalAttributeSet, MovementSpeed, OldMovementSpeed);
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
	if (Attribute == GetMovementSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, FLT_MAX);
	}
}

void UGYEnemyVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);


	UGYEnemyAbilitySystemComponent* EnemyASC = Cast<UGYEnemyAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (!EnemyASC) return;

	if (Data.EvaluatedData.Attribute == GetActivityPointsAttribute() && Data.EvaluatedData.Magnitude<0.f)
	{
		EnemyASC->ApplyActivityPointsUsedEffect();
	}

	if (Data.EvaluatedData.Attribute == GetMovementSpeedAttribute())
	{
		for (;;){
			AActor* OwningActor = GetOwningActor();
			if (!OwningActor) break;
			ACharacter* OwningChar = Cast<ACharacter>(OwningActor);
			if (!OwningChar) break;
			UGYCharacterMovementComponent* Move = Cast<UGYCharacterMovementComponent>(OwningChar->GetMovementComponent());
			if (!Move) break;
			Move->MaxWalkSpeed = GetMovementSpeed();
		}
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
