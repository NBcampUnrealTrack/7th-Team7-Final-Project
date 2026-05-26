#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYBaseAttribute::UGYBaseAttribute()
{
}

void UGYBaseAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYBaseAttribute, CurrentHealth);
	DOREPLIFETIME(UGYBaseAttribute, MaxHealth);
	DOREPLIFETIME(UGYBaseAttribute, Attack);
	DOREPLIFETIME(UGYBaseAttribute, Defense);
}

void UGYBaseAttribute::OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYBaseAttribute, CurrentHealth, OldCurrentHealth);
	UE_LOG(LogTemp, Warning, TEXT("[GYBaseAttribute] HP Replicated (클라) %.1f → %.1f — Owner: %s"),
		OldCurrentHealth.GetCurrentValue(),
		CurrentHealth.GetCurrentValue(),
		GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("Unknown"));
}

void UGYBaseAttribute::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYBaseAttribute, MaxHealth, OldMaxHealth);
}

void UGYBaseAttribute::OnRep_Attack(const FGameplayAttributeData& OldAttack)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYBaseAttribute, Attack, OldAttack);
}

void UGYBaseAttribute::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYBaseAttribute, Defense, OldDefense);
}

void UGYBaseAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UGYBaseAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GYBaseAttribute] HP 변경 (서버) %.1f / %.1f — Owner: %s"),
			GetCurrentHealth(), GetMaxHealth(),
			GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("Unknown"));
	}
}
