#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AttackLogic/Charge/GYChargeFragment.h"
#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"

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

	if (const UGYCollisionFragment* CF = Ability->GetFragment<UGYCollisionFragment>())
	{
		CachedCollisions = CF->GetBestMatchingShapes(OwnedTags);
		if (!CachedCollisions && !FallbackTags.IsEmpty())
		{
			CachedCollisions = CF->GetBestMatchingShapes(FallbackTags);
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

	if (ASC && ChargeData && ChargeData->ChargeCost.Attribute.IsValid() && ChargeData->ChargeCost.Amount > 0.f)
	{
		const float Current = ASC->GetNumericAttributeBase(ChargeData->ChargeCost.Attribute);
		ASC->SetNumericAttributeBase(ChargeData->ChargeCost.Attribute, FMath::Max(0.f, Current - ChargeData->ChargeCost.Amount));
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
			GYASC->NotifyAttributeChanged(ChargeData->ChargeCost.Attribute);
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

	if (const UGYChargeFragment* CostFragment = CachedAbility->GetFragment<UGYChargeFragment>())
	{
		const FGYChargeData* CostData = CostFragment->GetBestMatchingData(OwnedTags);
		if (!CostData && !FallbackTags.IsEmpty())
		{
			CostData = CostFragment->GetBestMatchingData(FallbackTags);
		}

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
			if (CostASC)
			{
				const float CostAmount = FMath::Lerp(0.f, CostData->AttackCost.Amount, Alpha);
				const float Current = CostASC->GetNumericAttributeBase(CostData->AttackCost.Attribute);
				CostASC->SetNumericAttributeBase(CostData->AttackCost.Attribute, FMath::Max(0.f, Current - CostAmount));
				if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(CostASC))
					GYASC->NotifyAttributeChanged(CostData->AttackCost.Attribute);
			}
		}
	}

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
