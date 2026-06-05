#include "AttackLogic/Parry/GYParryInputLogic.h"
#include "AttackLogic/Parry/GYParryFragment.h"
#include "AttackLogic/Parry/GYParryMontageFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"

using GYAttributeCostHelpers::ApplyCost;
using GYAttributeCostHelpers::ApplyReward;

void UGYParryInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedMontageSet = nullptr;
	CachedParryData = nullptr;

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

	if (const UGYParryMontageFragment* MF = Ability->GetFragment<UGYParryMontageFragment>())
	{
		CachedMontageSet = MF->GetBestMatchingSet(OwnedTags);
		if (!CachedMontageSet && !FallbackTags.IsEmpty())
		{
			CachedMontageSet = MF->GetBestMatchingSet(FallbackTags);
		}
	}

	if (const UGYParryFragment* PF = Ability->GetFragment<UGYParryFragment>())
	{
		CachedParryData = PF->GetBestMatchingData(OwnedTags);
		if (!CachedParryData && !FallbackTags.IsEmpty())
		{
			CachedParryData = PF->GetBestMatchingData(FallbackTags);
		}
	}

	if (!CachedMontageSet || !CachedParryData)
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

	ApplyCost(ASC, CachedParryData->StaminaCost);

	if (CachedMontageSet->ParryMontage)
	{
		Ability->PlayMontageForLogic(CachedMontageSet->ParryMontage, 1.f);
	}

	if (ASC)
	{
		for (const FGameplayTag& Tag : CachedParryData->ParryAppliedTags)
			ASC->AddLooseGameplayTag(Tag);
	}

	TWeakObjectPtr<UGYParryInputLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimer(
		ParryWindowTimer,
		[WeakThis]()
		{
			if (UGYParryInputLogic* Self = WeakThis.Get())
			{
				Self->OnParryWindowExpired();
			}
		},
		FMath::Max(CachedParryData->ParryTime, KINDA_SMALL_NUMBER),
		false
	);
}

void UGYParryInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	RemoveParryTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
	CachedParryData = nullptr;
}

TArray<FGameplayTag> UGYParryInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Parry_Hit };
}

void UGYParryInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag != GYGameplayTags::Event_Parry_Hit || !CachedAbility.IsValid()) return;

	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimer);
	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(ParryAnimTimer);

	RemoveParryTag();

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();

	if (CachedParryData)
	{
		ApplyReward(ASC, CachedParryData->StaminaReward);

		if (const AActor* AttackerActor = Payload.Instigator.Get())
		{
			if (UAbilitySystemComponent* AttackerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AttackerActor))
			{
				for (const FGYAttributeEffect& Effect : CachedParryData->ReceiverAffected)
					UGYAdditionalResourceStatics::ApplyAttributeDelta(AttackerASC, Effect.Attribute, Effect.Amount);
			}
		}
	}

	if (CachedMontageSet && CachedMontageSet->CounterMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->CounterMontage, 1.f);
		TWeakObjectPtr<UGYParryInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			CounterMontageTimer,
			[WeakThis]()
			{
				if (UGYParryInputLogic* Self = WeakThis.Get())
					if (Self->CachedAbility.IsValid())
						Self->CachedAbility->RequestEnd(false);
			},
			FMath::Max(Duration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

TArray<FGameplayTag> UGYParryInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Parry,
		GYGameplayTags::Ability_Fragment_ParryMontage
	};
}


void UGYParryInputLogic::OnParryWindowExpired()
{
	RemoveParryTag();

	if (!CachedAbility.IsValid() || !CachedParryData) return;

	TWeakObjectPtr<UGYParryInputLogic> WeakThis(this);
	CachedAbility->GetWorld()->GetTimerManager().SetTimer(
		ParryAnimTimer,
		[WeakThis]()
		{
			if (UGYParryInputLogic* Self = WeakThis.Get())
				Self->OnParryAnimExpired();
		},
		FMath::Max(CachedParryData->ParryAnimTime, KINDA_SMALL_NUMBER),
		false
	);
}

void UGYParryInputLogic::OnParryAnimExpired()
{
	PlayEndMontage();
}

void UGYParryInputLogic::PlayEndMontage()
{
	if (!CachedAbility.IsValid()) return;

	if (CachedMontageSet && CachedMontageSet->EndMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->EndMontage, 1.f);
		TWeakObjectPtr<UGYParryInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			EndMontageTimer,
			[WeakThis]()
			{
				if (UGYParryInputLogic* Self = WeakThis.Get())
				{
					if (Self->CachedAbility.IsValid())
					{
						Self->CachedAbility->RequestEnd(false);
					}
				}
			},
			FMath::Max(Duration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYParryInputLogic::RemoveParryTag()
{
	if (!CachedAbility.IsValid() || !CachedParryData) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;
	for (const FGameplayTag& Tag : CachedParryData->ParryAppliedTags)
	{
		if (ASC->HasMatchingGameplayTag(Tag))
			ASC->RemoveLooseGameplayTag(Tag);
	}
}
