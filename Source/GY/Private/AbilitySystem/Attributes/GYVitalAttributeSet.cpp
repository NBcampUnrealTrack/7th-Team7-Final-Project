#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Logging/GYLogManager.h"

namespace
{
	// 이 비율 이하에서 State.Life.LowHP 부여 (저체력 조건부 효과 기준선)
	constexpr float LowHPThreshold = 0.25f;
}

UGYVitalAttributeSet::UGYVitalAttributeSet()
{
	InitCurrentStagger(0.f);
	InitMaxStagger(0.f);
	InitCurrentStun(0.f);
	InitMaxStun(0.f);
	InitStaggerRecoveryRate(-0.075f);
	InitStunRecoveryRate(-0.04f);
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
	DOREPLIFETIME(UGYVitalAttributeSet, StaggerRecoveryRate);
	DOREPLIFETIME(UGYVitalAttributeSet, StunRecoveryRate);
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

void UGYVitalAttributeSet::OnRep_StaggerRecoveryRate(const FGameplayAttributeData& OldStaggerRecoveryRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, StaggerRecoveryRate, OldStaggerRecoveryRate);
}

void UGYVitalAttributeSet::OnRep_StunRecoveryRate(const FGameplayAttributeData& OldStunRecoveryRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGYVitalAttributeSet, StunRecoveryRate, OldStunRecoveryRate);
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
	else if (Attribute == GetMaxHealthAttribute())
	{
		// MaxHealth 감소(예: STR 하향) 시 CurrentHealth가 초과하지 않도록 클램프. 증가 시엔 여유만 늘어남.
		if (NewValue < GetCurrentHealth())
		{
			SetCurrentHealth(NewValue);
		}
	}
}

void UGYVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.f);
		if (LocalDamage > 0.f)
		{
			SetCurrentHealth(FMath::Clamp(GetCurrentHealth() - LocalDamage, 0.f, GetMaxHealth()));
		}
		UpdateLowHPState();
		return;
	}

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		GY_WARN(Combat, ESK, "HP 변경 (서버) %.1f / %.1f - Owner: %s",
			GetCurrentHealth(), GetMaxHealth(),
			GetOwningActor() ? *GetOwningActor()->GetName() : TEXT("Unknown"));
		UpdateLowHPState();
		return;
	}

	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		UpdateLowHPState();
		return;
	}

	float CurrentValue = 0.f;

	if (Data.EvaluatedData.Attribute == GetCurrentStaggerAttribute())
	{
		SetCurrentStagger(FMath::Clamp(GetCurrentStagger(), 0.f, GetMaxStagger()));
		CurrentValue = GetCurrentStagger();
		if (Data.EvaluatedData.Magnitude>0.f)
			HandleHitReaction(Data, Data.EvaluatedData.Magnitude);
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

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
	{
		GYASC->HandleVitalAccumulation(Data.EvaluatedData.Attribute, CurrentValue);

		if (Data.EvaluatedData.Magnitude > 0.f)
		{
			GYASC->NotifyAttributeChanged(Data.EvaluatedData.Attribute);
		}
	}
}

void UGYVitalAttributeSet::HandleHitReaction(const FGameplayEffectModCallbackData& Data, float DamageDone)
{
	AActor* SourceActor = Data.EffectSpec.GetContext().GetEffectCauser();
	AActor* TargetActor = GetOwningActor();
	if (!SourceActor || !TargetActor || SourceActor == TargetActor) return;

	UAbilitySystemComponent* TargetASC = Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
	if (!TargetASC) return;

	const FVector HitDir = (TargetActor->GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal();

	FGameplayCueParameters Params;
	Params.Normal = HitDir;
	Params.RawMagnitude = DamageDone * 100.f;
	Params.Instigator = SourceActor;
	Params.EffectCauser = TargetActor;
	Params.Location = TargetActor->GetActorLocation();

	TargetASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Combat_HitReaction, Params);
}

void UGYVitalAttributeSet::UpdateLowHPState()
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC) return;

	const float Max = GetMaxHealth();
	const bool bLow = Max > 0.f && GetCurrentHealth() <= Max * LowHPThreshold;
	const bool bHasTag = ASC->HasMatchingGameplayTag(GYStateTags::State_Life_LowHP);

	if (bLow && !bHasTag)
	{
		ASC->AddLooseGameplayTag(GYStateTags::State_Life_LowHP);
	}
	else if (!bLow && bHasTag)
	{
		ASC->RemoveLooseGameplayTag(GYStateTags::State_Life_LowHP);
	}
}
