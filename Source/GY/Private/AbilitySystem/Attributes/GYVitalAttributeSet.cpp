#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Logging/GYLogManager.h"

UGYVitalAttributeSet::UGYVitalAttributeSet()
{
	InitCurrentStagger(0.f);
	InitMaxStagger(0.f);
	InitCurrentStun(150.f);
	InitMaxStun(150.f);
}

void UGYVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYVitalAttributeSet, CurrentHealth);
	DOREPLIFETIME(UGYVitalAttributeSet, MaxHealth);
	DOREPLIFETIME(UGYVitalAttributeSet, CurrentStagger);
	DOREPLIFETIME(UGYVitalAttributeSet, MaxStagger);
	DOREPLIFETIME(UGYVitalAttributeSet, CurrentStun);
	DOREPLIFETIME(UGYVitalAttributeSet, MaxStun);
}

void UGYVitalAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, CurrentHealth, OldCurrentHealth);
	GY_WARN(Network, ESK, "HP Replicated (클라) %.1f -> %.1f - Owner: %s",
		OldCurrentHealth.GetCurrentValue(),
		CurrentHealth.GetCurrentValue(),
		GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("Unknown"));
}

void UGYVitalAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, MaxHealth, OldMaxHealth);
}

void UGYVitalAttributeSet::OnRep_CurrentStagger(const FGameplayAttributeData& OldCurrentStagger)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, CurrentStagger, OldCurrentStagger);
}

void UGYVitalAttributeSet::OnRep_MaxStagger(const FGameplayAttributeData& OldMaxStagger)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, MaxStagger, OldMaxStagger);
}

void UGYVitalAttributeSet::OnRep_CurrentStun(const FGameplayAttributeData& OldCurrentStun)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, CurrentStun, OldCurrentStun);
}

void UGYVitalAttributeSet::OnRep_MaxStun(const FGameplayAttributeData& OldMaxStun)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, MaxStun, OldMaxStun);
}

void UGYVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
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

void UGYVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		GY_WARN(Combat, ESK, "HP 변경 (서버) %.1f / %.1f - Owner: %s",
			GetCurrentHealth(), GetMaxHealth(),
			GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("Unknown"));
		return;
	}

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
