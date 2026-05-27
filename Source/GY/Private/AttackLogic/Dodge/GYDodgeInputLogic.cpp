#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/DirectionTags.h"

namespace
{
	FGameplayTag QuantizeAngleToDirectionTag(float AngleDegrees)
	{
		while (AngleDegrees >  180.f) AngleDegrees -= 360.f;
		while (AngleDegrees < -180.f) AngleDegrees += 360.f;

		const int32 Step = FMath::RoundToInt(AngleDegrees / 45.f);

		switch ((Step % 8 + 8) % 8)
		{
		case 0: return GYGameplayTags::Direction_Forward;
		case 1: return GYGameplayTags::Direction_ForwardRight;
		case 2: return GYGameplayTags::Direction_Right;
		case 3: return GYGameplayTags::Direction_BackRight;
		case 4: return GYGameplayTags::Direction_Back;
		case 5: return GYGameplayTags::Direction_BackLeft;
		case 6: return GYGameplayTags::Direction_Left;
		case 7: return GYGameplayTags::Direction_ForwardLeft;
		default: return GYGameplayTags::Direction_Forward;
		}
	}
}

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedMontageSet = nullptr;
	CachedDodgeData = nullptr;

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

	const FGameplayTag DirectionTag = QuantizeAngleToDirectionTag(0.f);

	if (const UGYDodgeMontageFragment* MF = Ability->GetFragment<UGYDodgeMontageFragment>())
	{
		CachedMontageSet = MF->GetMontageForWeaponAndDirection(OwnedTags, DirectionTag);
		if (!CachedMontageSet && !FallbackTags.IsEmpty())
		{
			CachedMontageSet = MF->GetMontageForWeaponAndDirection(FallbackTags, DirectionTag);
		}
	}

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		CachedDodgeData = DF->GetBestMatchingData(OwnedTags);
		if (!CachedDodgeData && !FallbackTags.IsEmpty())
		{
			CachedDodgeData = DF->GetBestMatchingData(FallbackTags);
		}
	}

	if (!CachedMontageSet || !CachedDodgeData)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get())
			{
				A->RequestEnd(true);
			}
		});
		return;
	}

	float MontageDuration = 0.f;
	if (CachedMontageSet->DodgeMontage)
	{
		MontageDuration = Ability->PlayMontageForLogic(CachedMontageSet->DodgeMontage, 1.f);
	}

	if (CachedDodgeData->AppliedTag.IsValid() && ASC)
	{
		ASC->AddLooseGameplayTag(CachedDodgeData->AppliedTag);
	}

	const float Ceiling = MontageDuration > 0.f
		? FMath::Min(CachedDodgeData->MaxDodgeTime, MontageDuration)
		: CachedDodgeData->MaxDodgeTime;
	const float EffectiveApplyTime = FMath::Clamp(CachedDodgeData->DodgeApplyTime, 0.f, Ceiling);

	TWeakObjectPtr<UGYDodgeInputLogic> WeakThis(this);

	if (EffectiveApplyTime > 0.f)
	{
		Ability->GetWorld()->GetTimerManager().SetTimer(
			TagTimer,
			[WeakThis]()
			{
				if (UGYDodgeInputLogic* Self = WeakThis.Get())
				{
					Self->OnTagWindowExpired();
				}
			},
			EffectiveApplyTime,
			false
		);
	}

	if (MontageDuration > 0.f)
	{
		Ability->GetWorld()->GetTimerManager().SetTimer(
			EndTimer,
			[WeakThis]()
			{
				if (UGYDodgeInputLogic* Self = WeakThis.Get())
				{
					Self->OnMontageExpired();
				}
			},
			MontageDuration,
			false
		);
	}
	else
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get())
			{
				A->RequestEnd(false);
			}
		});
	}
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	RemoveAppliedTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
	CachedDodgeData = nullptr;
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Dodge,
		GYGameplayTags::Ability_Fragment_DodgeMontage
	};
}

void UGYDodgeInputLogic::OnTagWindowExpired()
{
	RemoveAppliedTag();
}

void UGYDodgeInputLogic::OnMontageExpired()
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYDodgeInputLogic::RemoveAppliedTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeData) return;
	if (!CachedDodgeData->AppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeData->AppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeData->AppliedTag);
	}
}
