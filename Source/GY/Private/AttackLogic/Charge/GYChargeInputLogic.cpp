#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AttackLogic/Charge/GYChargeFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"

using GYAttributeCostHelpers::ApplyCost;

void UGYChargeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	bCharging = false;
	CachedMontageSet = nullptr;
	CachedCollisions = nullptr;
	ChargeStartTime = 0.f;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	if (const UGYChargeMontageFragment* MF = Ability->GetFragment<UGYChargeMontageFragment>())
	{
		CachedMontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (const UGYCollisionFragment* CF = Ability->GetFragment<UGYCollisionFragment>())
	{
		CachedCollisions = CF->GetBestMatchingShapes(OwnedTags);
	}

	const UGYChargeFragment* ChargeFragment = Ability->GetFragment<UGYChargeFragment>();
	const FGYChargeData* ChargeData = nullptr;
	if (ChargeFragment)
	{
		ChargeData = ChargeFragment->GetBestMatchingData(OwnedTags);
	}

	if (CachedMontageSet && CachedMontageSet->ChargeMontage)
	{
		Ability->PlayMontageForLogic(CachedMontageSet->ChargeMontage, 1.f);
	}

	if (ChargeData)
		ApplyCost(ASC, ChargeData->ChargeCost);

	bCharging = true;
	ChargeStartTime = Ability->GetWorld()->GetTimeSeconds();

	if (ChargeData && ChargeData->MaxChargeTime > 0.f)
	{
		MaxChargeTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(ChargeData->MaxChargeTime, KINDA_SMALL_NUMBER));
		MaxChargeTask->OnFinish.AddDynamic(this, &UGYChargeInputLogic::OnMaxChargeFinished);
		MaxChargeTask->ReadyForActivation();
	}
}

void UGYChargeInputLogic::CancelMaxChargeTimer()
{
	if (MaxChargeTask) { MaxChargeTask->EndTask(); MaxChargeTask = nullptr; }
}

void UGYChargeInputLogic::OnMaxChargeFinished()
{
	MaxChargeTask = nullptr;
	ExecuteAttack();
}

void UGYChargeInputLogic::OnAttackMontageFinished()
{
	MontageEndTask = nullptr;
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYChargeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (MaxChargeTask) { MaxChargeTask->EndTask(); MaxChargeTask = nullptr; }
	if (MontageEndTask) { MontageEndTask->EndTask(); MontageEndTask = nullptr; }
	bCharging = false;
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
	CachedCollisions = nullptr;
}

const FGYCollisionShapeData* UGYChargeInputLogic::GetCurrentCollisionData() const
{
	if (!CachedCollisions || CachedCollisions->IsEmpty()) return nullptr;
	return &(*CachedCollisions)[0];
}

TArray<FGameplayTag> UGYChargeInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Input_AttackRelease };
}

void UGYChargeInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag == GYGameplayTags::Event_Input_AttackRelease && bCharging)
	{
		ExecuteAttack();
	}
}

TArray<FGameplayTag> UGYChargeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Charge,
		GYGameplayTags::Ability_Fragment_ChargeMontage,
		GYGameplayTags::Ability_Fragment_Collision
	};
}

void UGYChargeInputLogic::ExecuteAttack()
{
	if (!CachedAbility.IsValid() || !bCharging) return;

	bCharging = false;
	if (MaxChargeTask) { MaxChargeTask->EndTask(); MaxChargeTask = nullptr; }

	if (UAbilitySystemComponent* ChargeEndASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Ability_Charge_Execute;
		ChargeEndASC->HandleGameplayEvent(GYGameplayTags::Event_Ability_Charge_Execute, &Payload);
	}

	const float ElapsedTime = CachedAbility->GetWorld()->GetTimeSeconds() - ChargeStartTime;

	FGYHitImpact Impact;

	FGameplayTagContainer OwnedTags;
	if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	if (const UGYChargeFragment* ChargeFragment = CachedAbility->GetFragment<UGYChargeFragment>())
	{
		const FGYChargeData* ChargeData = ChargeFragment->GetBestMatchingData(OwnedTags);

		if (ChargeData)
		{
			// 차지 Impact(최대치) 기준 + 멀티플라이어만 차지량으로 Lerp
			Impact = ChargeData->Impact;
			float Multiplier = 1.f;
			if (ElapsedTime >= ChargeData->MaxChargeTime)
			{
				Multiplier = ChargeData->Impact.DamageMultiplier;
			}
			else if (ElapsedTime >= ChargeData->MinChargeTime)
			{
				const float Range = FMath::Max(ChargeData->MaxChargeTime - ChargeData->MinChargeTime, KINDA_SMALL_NUMBER);
				const float Alpha = (ElapsedTime - ChargeData->MinChargeTime) / Range;
				Multiplier = FMath::Lerp(1.f, ChargeData->Impact.DamageMultiplier, Alpha);
			}
			Impact.DamageMultiplier = Multiplier;
		}
	}

	CachedAbility->SetCurrentHitImpact(Impact);

	if (const UGYChargeFragment* CostFragment = CachedAbility->GetFragment<UGYChargeFragment>())
	{
		const FGYChargeData* CostData = CostFragment->GetBestMatchingData(OwnedTags);

		if (CostData && CostData->AttackCost.Attribute.IsValid() && CostData->AttackCost.Amount > 0.f)
		{
			float Alpha = 0.f;
			if (ElapsedTime >= CostData->MaxChargeTime)
			{
				Alpha = 1.f;
			}
			else if (ElapsedTime >= CostData->MinChargeTime)
			{
				const float Range = FMath::Max(CostData->MaxChargeTime - CostData->MinChargeTime, KINDA_SMALL_NUMBER);
				Alpha = (ElapsedTime - CostData->MinChargeTime) / Range;
			}

			UAbilitySystemComponent* CostASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
			FGYAttributeCost LerpedCost;
			LerpedCost.Attribute = CostData->AttackCost.Attribute;
			LerpedCost.Amount = FMath::Lerp(0.f, CostData->AttackCost.Amount, Alpha);
			ApplyCost(CostASC, LerpedCost);
		}
	}

	if (CachedMontageSet && CachedMontageSet->AttackMontage)
	{
		const float MontageDuration = CachedAbility->PlayMontageForLogic(CachedMontageSet->AttackMontage, 1.f);

		MontageEndTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(MontageDuration, 0.1f));
		MontageEndTask->OnFinish.AddDynamic(this, &UGYChargeInputLogic::OnAttackMontageFinished);
		MontageEndTask->ReadyForActivation();
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}
