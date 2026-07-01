#include "AttackLogic/Shared/GYActiveTagLogic.h"
#include "AttackLogic/Shared/GYActiveTagFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYActiveTagLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;

	const UGYActiveTagFragment* Fragment = Ability->GetFragment<UGYActiveTagFragment>();
	if (!Fragment || Fragment->Tags.IsEmpty()) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	ASC->AddLooseGameplayTags(Fragment->Tags);
}

void UGYActiveTagLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	const UGYActiveTagFragment* Fragment = Ability->GetFragment<UGYActiveTagFragment>();
	if (Fragment && !Fragment->Tags.IsEmpty())
	{
		UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
		if (ASC) ASC->RemoveLooseGameplayTags(Fragment->Tags);
	}

	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYActiveTagLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ActiveTag };
}
