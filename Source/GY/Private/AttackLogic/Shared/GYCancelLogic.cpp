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

	return Fragment->CancelEventTags.GetGameplayTagArray();
}

void UGYCancelLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const UGYCancelFragment* Fragment = CachedAbility->GetFragment<UGYCancelFragment>();
	const float BlendOutTime = Fragment ? Fragment->MontageBlendOutTime : -1.f;

	if (BlendOutTime >= 0.f)
	{
		ASC->CurrentMontageStop(BlendOutTime);
	}
	CachedAbility->RequestEnd(true);
}

TArray<FGameplayTag> UGYCancelLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Cancel };
}
