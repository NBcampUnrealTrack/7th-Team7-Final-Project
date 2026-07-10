#include "AttackLogic/Shared/GYActiveTagLogic.h"
#include "AttackLogic/Shared/GYActiveTagFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYActiveTagLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	AppliedTags.Reset();

	const UGYActiveTagFragment* Fragment = Ability->GetFragment<UGYActiveTagFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGameplayTagContainer* TagsToApply = Fragment->GetBestMatchingTags(OwnedTags);
	if (!TagsToApply || TagsToApply->IsEmpty()) return;

	AppliedTags = *TagsToApply;
	ASC->AddLooseGameplayTags(AppliedTags);
}

void UGYActiveTagLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (!AppliedTags.IsEmpty())
	{
		UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
		if (ASC) ASC->RemoveLooseGameplayTags(AppliedTags);
		AppliedTags.Reset();
	}

	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYActiveTagLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ActiveTag };
}
