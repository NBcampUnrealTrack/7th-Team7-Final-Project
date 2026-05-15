#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYAdditionalAttribute::UGYAdditionalAttribute()
{
}

void UGYAdditionalAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYAdditionalAttribute, HitResistance);
	DOREPLIFETIME(UGYAdditionalAttribute, CurrentPoise);
	DOREPLIFETIME(UGYAdditionalAttribute, MaxPoise);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalRate);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalMultiplier);
}

void UGYAdditionalAttribute::OnRep_HitResistance(const FGameplayAttributeData& OldHitResistance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, HitResistance, OldHitResistance);
}

void UGYAdditionalAttribute::OnRep_CurrentPoise(const FGameplayAttributeData& OldCurrentPoise)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CurrentPoise, OldCurrentPoise);
}

void UGYAdditionalAttribute::OnRep_MaxPoise(const FGameplayAttributeData& OldMaxPoise)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, MaxPoise, OldMaxPoise);
}

void UGYAdditionalAttribute::OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CriticalRate, OldCriticalRate);
}

void UGYAdditionalAttribute::OnRep_CriticalMultiplier(const FGameplayAttributeData& OldCriticalMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CriticalMultiplier, OldCriticalMultiplier);
}

void UGYAdditionalAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UGYAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}
