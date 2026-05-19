#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYAdditionalAttribute::UGYAdditionalAttribute()
{
	InitCurrentHitRes(80.f);
	InitMaxHitRes(80.f);
	InitCurrentPoise(150.f);
	InitMaxPoise(150.f);
}

void UGYAdditionalAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYAdditionalAttribute, CurrentHitRes);
	DOREPLIFETIME(UGYAdditionalAttribute, MaxHitRes);
	DOREPLIFETIME(UGYAdditionalAttribute, CurrentPoise);
	DOREPLIFETIME(UGYAdditionalAttribute, MaxPoise);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalRate);
	DOREPLIFETIME(UGYAdditionalAttribute, CriticalMultiplier);
}

void UGYAdditionalAttribute::OnRep_CurrentHitRes(const FGameplayAttributeData& OldCurrentHitRes)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, CurrentHitRes, OldCurrentHitRes);
}

void UGYAdditionalAttribute::OnRep_MaxHitRes(const FGameplayAttributeData& OldMaxHitRes)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYAdditionalAttribute, MaxHitRes, OldMaxHitRes);
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

	if (Attribute == GetCurrentHitResAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHitRes());
	}
	else if (Attribute == GetCurrentPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
}

void UGYAdditionalAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHitResAttribute())
	{
		SetCurrentHitRes(FMath::Clamp(GetCurrentHitRes(), 0.f, GetMaxHitRes()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetCurrentPoiseAttribute())
	{
		SetCurrentPoise(FMath::Clamp(GetCurrentPoise(), 0.f, GetMaxPoise()));
		return;
	}
}
