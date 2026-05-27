#include "AttackLogic/Shared/GYCancelLogic.h"
#include "AttackLogic/Shared/GYCancelFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYCancelLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
}

void UGYCancelLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYCancelLogic::GetSubscribedEventTags() const
{
	if (!CachedAbility.IsValid()) return {};

	const UGYCancelFragment* Fragment = CachedAbility->GetFragment<UGYCancelFragment>();
	if (!Fragment) return {};

	TArray<FGameplayTag> Tags = Fragment->CancelEvents;

	for (const FGYCancelWindowEntry& Entry : Fragment->CancelWithinWindow)
	{
		if (Entry.EventTag.IsValid()) Tags.AddUnique(Entry.EventTag);
	}

	return Tags;
}

void UGYCancelLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const UGYCancelFragment* Fragment = CachedAbility->GetFragment<UGYCancelFragment>();
	if (!Fragment) return;

	if (Fragment->CancelEvents.Contains(EventTag))
	{
		DoCancel(ASC, Fragment);
		return;
	}

	bool bShouldCancel = false;
	for (const FGYCancelWindowEntry& Entry : Fragment->CancelWithinWindow)
	{
		if (Entry.EventTag == EventTag && Entry.WindowTag.IsValid() && ASC->HasMatchingGameplayTag(Entry.WindowTag))
		{
			bShouldCancel = true;
			break;
		}
	}

	if (bShouldCancel)
	{
		DoCancel(ASC, Fragment);
	}
}

TArray<FGameplayTag> UGYCancelLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Cancel };
}

void UGYCancelLogic::DoCancel(UAbilitySystemComponent* ASC, const UGYCancelFragment* Fragment)
{
	const float BlendOutTime = Fragment->MontageBlendOutTime;
	if (BlendOutTime >= 0.f)
	{
		ASC->CurrentMontageStop(BlendOutTime);
	}
	CachedAbility->RequestEnd(true);
}
