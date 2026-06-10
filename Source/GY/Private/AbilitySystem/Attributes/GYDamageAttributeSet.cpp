#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UGYDamageAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYDamageAttributeSet, Attack);
	DOREPLIFETIME(UGYDamageAttributeSet, Defense);
	DOREPLIFETIME(UGYDamageAttributeSet, CriticalRate);
	DOREPLIFETIME(UGYDamageAttributeSet, CriticalMultiplier);
}

void UGYDamageAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldAttack)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYDamageAttributeSet, Attack, OldAttack);
}

void UGYDamageAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYDamageAttributeSet, Defense, OldDefense);
}

void UGYDamageAttributeSet::OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYDamageAttributeSet, CriticalRate, OldCriticalRate);
}

void UGYDamageAttributeSet::OnRep_CriticalMultiplier(const FGameplayAttributeData& OldCriticalMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYDamageAttributeSet, CriticalMultiplier, OldCriticalMultiplier);
}
