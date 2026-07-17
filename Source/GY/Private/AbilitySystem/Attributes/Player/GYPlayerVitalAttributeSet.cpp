#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

namespace
{
	constexpr float StaminaRegenDefaultPerTick = 10.f;   // 스태미나 0 이상: 기본 회복
	constexpr float StaminaRegenNegativePerTick = 1.0f;  // 과소모(음수): 느린 회복
}

UGYPlayerVitalAttributeSet::UGYPlayerVitalAttributeSet()
{
	InitCurrentHealth(100.f);
	InitMaxHealth(100.f);
	InitCurrentStamina(100.f);
	InitMaxStamina(100.f);
	InitStaminaRegenPerTick(StaminaRegenDefaultPerTick);
}

void UGYPlayerVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYPlayerVitalAttributeSet, CurrentStamina);
	DOREPLIFETIME(UGYPlayerVitalAttributeSet, MaxStamina);
	DOREPLIFETIME(UGYPlayerVitalAttributeSet, StaminaRegenPerTick);
}

void UGYPlayerVitalAttributeSet::OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerVitalAttributeSet, CurrentStamina, OldCurrentStamina);
}

void UGYPlayerVitalAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerVitalAttributeSet, MaxStamina, OldMaxStamina);
}

void UGYPlayerVitalAttributeSet::OnRep_StaminaRegenPerTick(const FGameplayAttributeData& OldStaminaRegenPerTick)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYPlayerVitalAttributeSet, StaminaRegenPerTick, OldStaminaRegenPerTick);
}

void UGYPlayerVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetCurrentStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, -GetMaxStamina(), GetMaxStamina());
	}
}

void UGYPlayerVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetCurrentStaminaAttribute())
	{
		SetCurrentStamina(FMath::Clamp(GetCurrentStamina(), -GetMaxStamina(), GetMaxStamina()));

		// 과소모(음수)면 회복 base를 느린 값으로, 0 이상 복귀면 기본값으로 전환. 인첸트 등 modifier는 base 위에 유지된다.
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			const FGameplayAttribute RegenAttr = GetStaminaRegenPerTickAttribute();
			const float TargetBase = (GetCurrentStamina() < 0.f) ? StaminaRegenNegativePerTick : StaminaRegenDefaultPerTick;
			if (!FMath::IsNearlyEqual(ASC->GetNumericAttributeBase(RegenAttr), TargetBase))
			{
				ASC->SetNumericAttributeBase(RegenAttr, TargetBase);
			}
		}

		// 소모(음수)일 때만 회복 딜레이 리셋 — 회복 GE(양수)에 걸면 regen이 자기 딜레이를 리셋해 영영 안 참
		if (Data.EvaluatedData.Magnitude < 0.f)
		{
			if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
				GYASC->NotifyAttributeChanged(GetCurrentStaminaAttribute());
		}
	}
}
