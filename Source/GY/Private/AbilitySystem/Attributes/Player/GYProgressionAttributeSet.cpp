#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UGYProgressionAttributeSet::UGYProgressionAttributeSet()
{
	InitLevel(1.f);
	InitSkillPoint(0.f);
	InitXP(0.f);
}

void UGYProgressionAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGYProgressionAttributeSet, Level);
	DOREPLIFETIME(UGYProgressionAttributeSet, SkillPoint);
	DOREPLIFETIME(UGYProgressionAttributeSet, XP);
}

void UGYProgressionAttributeSet::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYProgressionAttributeSet, Level, OldLevel);
}

void UGYProgressionAttributeSet::OnRep_SkillPoint(const FGameplayAttributeData& OldSkillPoint)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYProgressionAttributeSet, SkillPoint, OldSkillPoint);
}

void UGYProgressionAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYProgressionAttributeSet, XP, OldXP);
}

void UGYProgressionAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetXPAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetLevelAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetSkillPointAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UGYProgressionAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

	if (Data.EvaluatedData.Attribute == GetXPAttribute())
	{
		if (NextLevelXPCurve && LevelUpEffect)
		{
			float MinTime, MaxTime;
			NextLevelXPCurve->GetTimeRange(MinTime, MaxTime);

			while (GetLevel() <= MaxTime && GetXP() >= NextLevelXPCurve->GetFloatValue(GetLevel()))
			{
				const float Threshold = NextLevelXPCurve->GetFloatValue(GetLevel());
				SetXP(GetXP() - Threshold);

				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(LevelUpEffect, GetLevel(), Context);
				if (SpecHandle.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
				}
			}
		}
		return;
	}

	if (Data.EvaluatedData.Attribute == GetLevelAttribute())
	{
		// 레벨업시 전부 회복
		UGYPlayerVitalAttributeSet* Vital = const_cast<UGYPlayerVitalAttributeSet*>(ASC->GetSet<UGYPlayerVitalAttributeSet>());
		if (Vital)
		{
			Vital->SetCurrentHealth(Vital->GetMaxHealth());
			Vital->SetCurrentStagger(Vital->GetMaxStagger());
			Vital->SetCurrentStun(Vital->GetMaxStun());
			Vital->SetCurrentStamina(Vital->GetMaxStamina());
		}
		return;
	}
}
