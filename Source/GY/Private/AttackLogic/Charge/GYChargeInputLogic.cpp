#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AttackLogic/Charge/GYChargeFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYChargeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	bCharging = false;
	CachedMontageSet = nullptr;
	ChargeStartTime = 0.f;

	FGameplayTagContainer OwnedTags;
	if (UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	FGameplayTagContainer FallbackTags;
	if (Ability->DefaultWeaponTypeTag.IsValid())
	{
		FallbackTags.AddTag(Ability->DefaultWeaponTypeTag);
	}

	if (const UGYChargeMontageFragment* MF = Ability->GetFragment<UGYChargeMontageFragment>())
	{
		CachedMontageSet = MF->GetBestMatchingSet(OwnedTags);
		if (!CachedMontageSet && !FallbackTags.IsEmpty())
		{
			CachedMontageSet = MF->GetBestMatchingSet(FallbackTags);
		}
	}

	const UGYChargeFragment* ChargeFragment = Ability->GetFragment<UGYChargeFragment>();
	const FGYChargeData* ChargeData = nullptr;
	if (ChargeFragment)
	{
		ChargeData = ChargeFragment->GetBestMatchingData(OwnedTags);
		if (!ChargeData && !FallbackTags.IsEmpty())
		{
			ChargeData = ChargeFragment->GetBestMatchingData(FallbackTags);
		}
	}

	if (CachedMontageSet && CachedMontageSet->ChargeMontage)
	{
		Ability->PlayMontageForLogic(CachedMontageSet->ChargeMontage, 1.f);
	}

	bCharging = true;
	ChargeStartTime = Ability->GetWorld()->GetTimeSeconds();

	if (ChargeData && ChargeData->MaxChargeTime > 0.f)
	{
		TWeakObjectPtr<UGYChargeInputLogic> WeakThis(this);
		Ability->GetWorld()->GetTimerManager().SetTimer(
			MaxChargeTimer,
			[WeakThis]()
			{
				if (UGYChargeInputLogic* Self = WeakThis.Get())
				{
					Self->ExecuteAttack();
				}
			},
			ChargeData->MaxChargeTime,
			false
		);
	}
}

void UGYChargeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	bCharging = false;
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
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
	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(MaxChargeTimer);

	const float ElapsedTime = CachedAbility->GetWorld()->GetTimeSeconds() - ChargeStartTime;

	float DamageMultiplier = 1.f;

	FGameplayTagContainer OwnedTags;
	if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	FGameplayTagContainer FallbackTags;
	if (CachedAbility->DefaultWeaponTypeTag.IsValid())
	{
		FallbackTags.AddTag(CachedAbility->DefaultWeaponTypeTag);
	}

	if (const UGYChargeFragment* ChargeFragment = CachedAbility->GetFragment<UGYChargeFragment>())
	{
		const FGYChargeData* ChargeData = ChargeFragment->GetBestMatchingData(OwnedTags);
		if (!ChargeData && !FallbackTags.IsEmpty())
		{
			ChargeData = ChargeFragment->GetBestMatchingData(FallbackTags);
		}

		if (ChargeData)
		{
			if (ElapsedTime >= ChargeData->MaxChargeTime)
			{
				DamageMultiplier = ChargeData->DamageMultiplier;
			}
			else if (ElapsedTime >= ChargeData->MinChargeTime)
			{
				const float Range = FMath::Max(ChargeData->MaxChargeTime - ChargeData->MinChargeTime, KINDA_SMALL_NUMBER);
				const float Alpha = (ElapsedTime - ChargeData->MinChargeTime) / Range;
				DamageMultiplier = FMath::Lerp(1.f, ChargeData->DamageMultiplier, Alpha);
			}
		}
	}

	CachedAbility->SetDamageMultiplier(DamageMultiplier);

	if (CachedMontageSet && CachedMontageSet->AttackMontage)
	{
		const float MontageDuration = CachedAbility->PlayMontageForLogic(CachedMontageSet->AttackMontage, 1.f);

		TWeakObjectPtr<UGYChargeInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			MontageEndTimer,
			[WeakThis]()
			{
				if (UGYChargeInputLogic* Self = WeakThis.Get())
				{
					if (Self->CachedAbility.IsValid())
					{
						Self->CachedAbility->RequestEnd(false);
					}
				}
			},
			FMath::Max(MontageDuration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}
