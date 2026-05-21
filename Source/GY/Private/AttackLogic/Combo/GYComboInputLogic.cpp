#include "AttackLogic/Combo/GYComboInputLogic.h"
#include "AttackLogic/Combo/GYComboFragment.h"
#include "AttackLogic/Combo/GYComboAnimDataAsset.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYComboInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	ComboIndex = 0;
	bWindowOpen = false;
	bPendingCombo = false;
	CachedAnimSet = nullptr;

	const UGYComboFragment* Fragment = Ability->GetFragment<UGYComboFragment>();
	if (!Fragment) return;

	MaxComboCount = FMath::Max(1, FMath::FloorToInt(Fragment->ComboCount));

	if (AnimDataAsset)
	{
		FGameplayTagContainer OwnedTags;
		if (UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo())
		{
			ASC->GetOwnedGameplayTags(OwnedTags);
		}
		CachedAnimSet = AnimDataAsset->GetBestMatchingAnimSet(OwnedTags, DefaultAnimSetTag);
		if (CachedAnimSet)
		{
			MaxComboCount = FMath::Min(MaxComboCount, CachedAnimSet->Hits.Num());
		}
	}

	bReady = false;
	TWeakObjectPtr<UGYComboInputLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (UGYComboInputLogic* Self = WeakThis.Get())
		{
			Self->bReady = true;
		}
	});

	PlayCurrentMontage();
}

void UGYComboInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	CachedAbility.Reset();
	CachedAnimSet = nullptr;
	bWindowOpen = false;
	bPendingCombo = false;
	bReady = false;
}

TArray<FGameplayTag> UGYComboInputLogic::GetSubscribedEventTags() const
{
	return {
		GYGameplayTags::Event_Input_Attack,
		GYGameplayTags::Event_Anim_ComboWindowOpen,
		GYGameplayTags::Event_Anim_ComboWindowClose
	};
}

void UGYComboInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;

	if (EventTag == GYGameplayTags::Event_Input_Attack)
	{
		if (!bReady) return;

		if (bWindowOpen)
		{
			AdvanceCombo();
		}
		else
		{
			bPendingCombo = true;
		}
	}
	else if (EventTag == GYGameplayTags::Event_Anim_ComboWindowOpen)
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

TArray<FGameplayTag> UGYComboInputLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Attack };
}

void UGYComboInputLogic::PlayCurrentMontage()
{
	if (!CachedAbility.IsValid() || !CachedAnimSet) return;
	if (!CachedAnimSet->Hits.IsValidIndex(ComboIndex)) return;

	UAnimMontage* Montage = CachedAnimSet->Hits[ComboIndex].Montage.Get();
	if (Montage)
	{
		CachedAbility->PlayMontageForLogic(Montage, 1.f);
	}
}

void UGYComboInputLogic::AdvanceCombo()
{
	if (ComboIndex < MaxComboCount - 1)
	{
		ComboIndex++;
		bWindowOpen = false;
		bPendingCombo = false;
		PlayCurrentMontage();
	}
}

const FComboHitData* UGYComboInputLogic::GetCurrentHitData() const
{
	if (!CachedAnimSet || !CachedAnimSet->Hits.IsValidIndex(ComboIndex)) return nullptr;
	return &CachedAnimSet->Hits[ComboIndex];
}
