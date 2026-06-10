#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "Net/UnrealNetwork.h"

UGYCoreStatAttributeSet::UGYCoreStatAttributeSet()
{
	InitStrength(0.f);
	InitDexterity(0.f);
	InitEvasionInvincibilityTime(0.2f);
}

void UGYCoreStatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYCoreStatAttributeSet, Strength);
	DOREPLIFETIME(UGYCoreStatAttributeSet, Dexterity);
	DOREPLIFETIME(UGYCoreStatAttributeSet, EvasionInvincibilityTime);
}

void UGYCoreStatAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, Strength, OldStrength);
}

void UGYCoreStatAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldDexterity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, Dexterity, OldDexterity);
}

void UGYCoreStatAttributeSet::OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYCoreStatAttributeSet, EvasionInvincibilityTime, OldEvasionInvincibilityTime);
}
