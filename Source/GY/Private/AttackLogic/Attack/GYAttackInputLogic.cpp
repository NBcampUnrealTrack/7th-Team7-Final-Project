#include "AttackLogic/Attack/GYAttackInputLogic.h"

#include "AttackLogic/Combo/GYComboFragment.h"
#include "AttackLogic/Combo/GYComboMontageFragment.h"
#include "AttackLogic/Charge/GYChargeFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "TimerManager.h"

namespace
{
	void GatherTags(UGYPlayerGameplayAbility* Ability, FGameplayTagContainer& OutOwned, FGameplayTagContainer& OutFallback)
	{
		if (UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo())
		{
			ASC->GetOwnedGameplayTags(OutOwned);
		}
		if (Ability->DefaultWeaponTypeTag.IsValid())
		{
			OutFallback.AddTag(Ability->DefaultWeaponTypeTag);
		}
	}
}

void UGYAttackInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	ComboIndex = 0;
	bWindowOpen = false;
	bPendingCombo = false;
	bCharging = false;
	bReady = false;
	bComboStarted = false;
	// 활성화 자체가 눌린 입력으로부터 시작되므로, 같은 프레임 직후 들어올 InputPressed는 소진된 것으로 본다.
	bInputHeld = true;
	CachedComboMontages = nullptr;
	CachedComboCollisions = nullptr;
	CachedChargeMontageSet = nullptr;
	CachedChargeData = nullptr;

	FGameplayTagContainer OwnedTags;
	FGameplayTagContainer FallbackTags;
	GatherTags(Ability, OwnedTags, FallbackTags);

	const UGYComboFragment* ComboFrag = Ability->GetFragment<UGYComboFragment>();
	MaxComboCount = ComboFrag ? FMath::Max(1, FMath::FloorToInt(ComboFrag->ComboCount)) : 1;

	if (const UGYComboMontageFragment* MF = Ability->GetFragment<UGYComboMontageFragment>())
	{
		CachedComboMontages = MF->GetBestMatchingMontages(OwnedTags);
		if (!CachedComboMontages && !FallbackTags.IsEmpty())
		{
			CachedComboMontages = MF->GetBestMatchingMontages(FallbackTags);
		}
		if (CachedComboMontages)
		{
			MaxComboCount = FMath::Min(MaxComboCount, CachedComboMontages->Num());
		}
	}

	if (const UGYCollisionFragment* CF = Ability->GetFragment<UGYCollisionFragment>())
	{
		CachedComboCollisions = CF->GetBestMatchingShapes(OwnedTags);
		if (!CachedComboCollisions && !FallbackTags.IsEmpty())
		{
			CachedComboCollisions = CF->GetBestMatchingShapes(FallbackTags);
		}
	}
	CachedChargeCollisions = CachedComboCollisions;

	if (const UGYChargeMontageFragment* CMF = Ability->GetFragment<UGYChargeMontageFragment>())
	{
		CachedChargeMontageSet = CMF->GetBestMatchingSet(OwnedTags);
		if (!CachedChargeMontageSet && !FallbackTags.IsEmpty())
		{
			CachedChargeMontageSet = CMF->GetBestMatchingSet(FallbackTags);
		}
	}

	if (const UGYChargeFragment* ChargeFrag = Ability->GetFragment<UGYChargeFragment>())
	{
		CachedChargeData = ChargeFrag->GetBestMatchingData(OwnedTags);
		if (!CachedChargeData && !FallbackTags.IsEmpty())
		{
			CachedChargeData = ChargeFrag->GetBestMatchingData(FallbackTags);
		}
	}

	if (!CachedComboMontages)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get()) A->RequestEnd(true);
		});
		return;
	}

	TWeakObjectPtr<UGYAttackInputLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (UGYAttackInputLogic* Self = WeakThis.Get()) Self->bReady = true;
	});

	// 첫타는 즉시 내지 않는다: 임계 전에 떼면 콤보(탭), 임계 이상 홀드면 차지.
	if (HoldToChargeTime > 0.f)
	{
		Ability->GetWorld()->GetTimerManager().SetTimer(ChargeThresholdTimer,
			[WeakThis]() { if (UGYAttackInputLogic* Self = WeakThis.Get()) Self->OnChargeThresholdReached(); },
			HoldToChargeTime, false);
	}
	else
	{
		// 차지 비활성 → 누름 즉시 콤보 시작
		StartCombo();
	}
}

void UGYAttackInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	CachedAbility.Reset();
	CachedComboMontages = nullptr;
	CachedComboCollisions = nullptr;
	CachedChargeMontageSet = nullptr;
	CachedChargeData = nullptr;
	CachedChargeCollisions = nullptr;
	bWindowOpen = false;
	bPendingCombo = false;
	bCharging = false;
	bReady = false;
	bInputHeld = false;
}

TArray<FGameplayTag> UGYAttackInputLogic::GetSubscribedEventTags() const
{
	return {
		GYGameplayTags::Event_Anim_ComboWindowOpen,
		GYGameplayTags::Event_Anim_ComboWindowClose
	};
}

void UGYAttackInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid() || bCharging) return;

	if (EventTag == GYGameplayTags::Event_Anim_ComboWindowOpen)
	{
		bWindowOpen = true;
		ComboIndexAtWindowOpen = ComboIndex;
		if (bPendingCombo)
		{
			bPendingCombo = false;
			AdvanceCombo();
		}
	}
	else if (EventTag == GYGameplayTags::Event_Anim_ComboWindowClose)
	{
		bWindowOpen = false;
		bPendingCombo = false;
		if (ComboIndex == ComboIndexAtWindowOpen)
		{
			CachedAbility->RequestEnd(false);
		}
	}
}

TArray<FGameplayTag> UGYAttackInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Attack,
		GYGameplayTags::Ability_Fragment_ComboMontage,
		GYGameplayTags::Ability_Fragment_Charge,
		GYGameplayTags::Ability_Fragment_ChargeMontage,
		GYGameplayTags::Ability_Fragment_Collision
	};
}

const FGYCollisionShapeData* UGYAttackInputLogic::GetCurrentCollisionData() const
{
	if (bCharging)
	{
		if (!CachedChargeCollisions || CachedChargeCollisions->IsEmpty()) return nullptr;
		return &(*CachedChargeCollisions)[0];
	}

	if (!CachedComboCollisions || !CachedComboCollisions->IsValidIndex(ComboIndex)) return nullptr;
	return &(*CachedComboCollisions)[ComboIndex];
}

void UGYAttackInputLogic::OnInputPressed()
{
	// 누름 엣지에서만 1회 처리 (누르고 있는 동안의 반복 호출 무시)
	if (bInputHeld) return;
	bInputHeld = true;

	if (!CachedAbility.IsValid() || bCharging) return;

	// 콤보가 시작된 뒤의 재입력만 다음 단계로 진행시킨다.
	// (첫 입력의 탭/홀드 판정은 OnInputReleased / OnChargeThresholdReached가 담당)
	if (bReady && bComboStarted)
	{
		if (bWindowOpen)
		{
			AdvanceCombo();
		}
		else
		{
			bPendingCombo = true;
		}
	}
}

void UGYAttackInputLogic::OnInputReleased()
{
	bInputHeld = false;

	if (!CachedAbility.IsValid()) return;

	if (bCharging)
	{
		ExecuteChargeAttack();
	}
	else if (!bComboStarted)
	{
		// 임계 전에 뗌 → 탭 → 첫 콤보 시작 (차지 취소)
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(ChargeThresholdTimer);
		StartCombo();
	}
}

void UGYAttackInputLogic::OnChargeThresholdReached()
{
	if (!CachedAbility.IsValid() || bCharging || bComboStarted) return;
	EnterCharge();
}

void UGYAttackInputLogic::StartCombo()
{
	if (bComboStarted) return;
	bComboStarted = true;
	PlayComboMontage();
}

void UGYAttackInputLogic::PlayComboMontage()
{
	if (!CachedAbility.IsValid() || !CachedComboMontages) return;
	if (!CachedComboMontages->IsValidIndex(ComboIndex)) return;

	if (const UGYComboFragment* Fragment = CachedAbility->GetFragment<UGYComboFragment>())
	{
		FGameplayTagContainer OwnedTags;
		FGameplayTagContainer FallbackTags;
		GatherTags(CachedAbility.Get(), OwnedTags, FallbackTags);

		const TArray<FGYComboStepData>* Steps = Fragment->GetBestMatchingSteps(OwnedTags);
		if (!Steps && !FallbackTags.IsEmpty())
		{
			Steps = Fragment->GetBestMatchingSteps(FallbackTags);
		}

		float Multiplier = 1.f;
		if (Steps && Steps->IsValidIndex(ComboIndex))
		{
			const FGYComboStepData& Step = (*Steps)[ComboIndex];
			Multiplier = Step.DamageMultiplier;

			if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
			{
				if (Step.StaminaCost.Attribute.IsValid() && Step.StaminaCost.Amount > 0.f)
				{
					const float Current = ASC->GetNumericAttributeBase(Step.StaminaCost.Attribute);
					ASC->SetNumericAttributeBase(Step.StaminaCost.Attribute, FMath::Max(0.f, Current - Step.StaminaCost.Amount));
				}
			}
		}
		CachedAbility->SetDamageMultiplier(Multiplier);
	}

	if (UAnimMontage* Montage = (*CachedComboMontages)[ComboIndex])
	{
		CachedAbility->PlayMontageForLogic(Montage, 1.f);
	}
}

void UGYAttackInputLogic::AdvanceCombo()
{
	if (ComboIndex < MaxComboCount - 1)
	{
		ComboIndex++;
		bWindowOpen = false;
		bPendingCombo = false;
		PlayComboMontage();
	}
}

void UGYAttackInputLogic::EnterCharge()
{
	if (!CachedAbility.IsValid()) return;

	bCharging = true;
	bWindowOpen = false;
	bPendingCombo = false;
	ChargeStartTime = CachedAbility->GetWorld()->GetTimeSeconds();

	if (CachedChargeMontageSet && CachedChargeMontageSet->ChargeMontage)
	{
		CachedAbility->PlayMontageForLogic(CachedChargeMontageSet->ChargeMontage, 1.f);
	}

	if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		if (CachedChargeData && CachedChargeData->ChargeCost.Attribute.IsValid() && CachedChargeData->ChargeCost.Amount > 0.f)
		{
			const float Current = ASC->GetNumericAttributeBase(CachedChargeData->ChargeCost.Attribute);
			ASC->SetNumericAttributeBase(CachedChargeData->ChargeCost.Attribute, FMath::Max(0.f, Current - CachedChargeData->ChargeCost.Amount));
		}
	}

	if (CachedChargeData && CachedChargeData->MaxChargeTime > 0.f)
	{
		TWeakObjectPtr<UGYAttackInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(MaxChargeTimer,
			[WeakThis]() { if (UGYAttackInputLogic* Self = WeakThis.Get()) Self->ExecuteChargeAttack(); },
			CachedChargeData->MaxChargeTime, false);
	}
}

void UGYAttackInputLogic::ExecuteChargeAttack()
{
	if (!CachedAbility.IsValid() || !bCharging) return;

	bCharging = false;
	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(MaxChargeTimer);

	const float ElapsedTime = CachedAbility->GetWorld()->GetTimeSeconds() - ChargeStartTime;

	float DamageMultiplier = 1.f;
	float CostAlpha = 0.f;
	if (CachedChargeData)
	{
		if (ElapsedTime >= CachedChargeData->MaxChargeTime)
		{
			DamageMultiplier = CachedChargeData->DamageMultiplier;
			CostAlpha = 1.f;
		}
		else if (ElapsedTime >= CachedChargeData->MinChargeTime)
		{
			const float Range = FMath::Max(CachedChargeData->MaxChargeTime - CachedChargeData->MinChargeTime, KINDA_SMALL_NUMBER);
			const float Alpha = (ElapsedTime - CachedChargeData->MinChargeTime) / Range;
			DamageMultiplier = FMath::Lerp(1.f, CachedChargeData->DamageMultiplier, Alpha);
			CostAlpha = Alpha;
		}
	}

	CachedAbility->SetDamageMultiplier(DamageMultiplier);

	if (CachedChargeData && CachedChargeData->AttackCost.Attribute.IsValid() && CachedChargeData->AttackCost.Amount > 0.f)
	{
		if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
		{
			const float CostAmount = FMath::Lerp(0.f, CachedChargeData->AttackCost.Amount, CostAlpha);
			const float Current = ASC->GetNumericAttributeBase(CachedChargeData->AttackCost.Attribute);
			ASC->SetNumericAttributeBase(CachedChargeData->AttackCost.Attribute, FMath::Max(0.f, Current - CostAmount));
		}
	}

	if (CachedChargeMontageSet && CachedChargeMontageSet->AttackMontage)
	{
		const float MontageDuration = CachedAbility->PlayMontageForLogic(CachedChargeMontageSet->AttackMontage, 1.f);

		TWeakObjectPtr<UGYAttackInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(MontageEndTimer,
			[WeakThis]()
			{
				if (UGYAttackInputLogic* Self = WeakThis.Get())
				{
					if (Self->CachedAbility.IsValid()) Self->CachedAbility->RequestEnd(false);
				}
			},
			FMath::Max(MontageDuration, 0.1f), false);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}
