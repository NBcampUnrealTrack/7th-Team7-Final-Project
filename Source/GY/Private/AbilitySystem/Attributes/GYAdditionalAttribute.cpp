#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYAdditionalAttribute::UGYAdditionalAttribute()
{
	InitCurrentStagger(80.f);
	InitMaxStagger(80.f);
	InitCurrentStun(150.f);
	InitMaxStun(150.f);
}

void UGYAdditionalAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYAdditionalAttribute, CurrentStagger);
	DOREPLIFETIME(UGYAdditionalAttribute, MaxStagger);
	DOREPLIFETIME(UGYAdditionalAttribute, CurrentStun);
	DOREPLIFETIME(UGYAdditionalAttribute, MaxStun);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalRate);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalMultiplier);
}

void UGYAdditionalAttribute::OnRep_CurrentStagger(const FGameplayAttributeData& OldCurrentStagger)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CurrentStagger, OldCurrentStagger);
}

void UGYAdditionalAttribute::OnRep_MaxStagger(const FGameplayAttributeData& OldMaxStagger)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, MaxStagger, OldMaxStagger);
}

void UGYAdditionalAttribute::OnRep_CurrentStun(const FGameplayAttributeData& OldCurrentStun)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CurrentStun, OldCurrentStun);
}

void UGYAdditionalAttribute::OnRep_MaxStun(const FGameplayAttributeData& OldMaxStun)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, MaxStun, OldMaxStun);
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

	if (Attribute == GetCurrentStaggerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStagger());
	}
	else if (Attribute == GetCurrentStunAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStun());
	}
}

void UGYAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
	{
		SetCurrentStagger(FMath::Clamp(GetCurrentStagger(), 0.f, GetMaxStagger()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetCurrentStunAttribute())
	{
		SetCurrentStun(FMath::Clamp(GetCurrentStun(), 0.f, GetMaxStun()));
		return;
	}
}
