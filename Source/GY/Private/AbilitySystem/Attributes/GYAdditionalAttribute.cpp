#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYAdditionalAttribute::UGYAdditionalAttribute()
{
	InitCurrentStagger(0.f);
	InitMaxStagger(0.f);
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

	float CurrentValue = 0.f;

	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
	{
		SetCurrentStagger(FMath::Clamp(GetCurrentStagger(), 0.f, GetMaxStagger()));
		CurrentValue = GetCurrentStagger();
	}
	else if (Data.EvaluatedData.Attribute == GetCurrentStunAttribute())
	{
		SetCurrentStun(FMath::Clamp(GetCurrentStun(), 0.f, GetMaxStun()));
		CurrentValue = GetCurrentStun();
	}
	else
	{
		return;
	}

	UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (!GYASC) return;

	for (const FGYAttributeThresholdEvent& Entry : GYASC->AttributeThresholdEvents)
	{
		if (Entry.Attribute != Data.EvaluatedData.Attribute || !Entry.EventTag.IsValid()) continue;

		bool bFire = false;
		if (Entry.Threshold == EGYAttributeThreshold::AtMax)
		{
			if (Entry.MaxAttribute.IsValid())
			{
				const float MaxValue = GYASC->GetNumericAttributeBase(Entry.MaxAttribute);
				bFire = MaxValue > 0.f && CurrentValue >= MaxValue;
			}
		}
		else
		{
			bFire = CurrentValue <= 0.f;
		}

		if (bFire)
		{
			FGameplayEventData Payload;
			GYASC->Multicast_SendGameplayEvent(Entry.EventTag, Payload);
		}
	}
}
