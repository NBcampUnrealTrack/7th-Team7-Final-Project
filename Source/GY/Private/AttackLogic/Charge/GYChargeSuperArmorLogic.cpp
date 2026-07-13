#include "AttackLogic/Charge/GYChargeSuperArmorLogic.h"
#include "AttackLogic/Charge/GYChargeSuperArmorFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

void UGYChargeSuperArmorLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	bTagApplied = false;

	const UGYChargeSuperArmorFragment* Fragment = Ability->GetFragment<UGYChargeSuperArmorFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	if (!Fragment->IsActive(OwnedTags)) return;

	ASC->AddLooseGameplayTag(GYStateTags::State_Combat_SuperArmor);
	bTagApplied = true;
}

void UGYChargeSuperArmorLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	RemoveSuperArmorTag();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYChargeSuperArmorLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Ability_Charge_Execute };
}

void UGYChargeSuperArmorLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag != GYGameplayTags::Event_Ability_Charge_Execute) return;

	RemoveSuperArmorTag();
}

TArray<FGameplayTag> UGYChargeSuperArmorLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ChargeSuperArmor };
}

void UGYChargeSuperArmorLogic::RemoveSuperArmorTag()
{
	if (!bTagApplied || !CachedAbility.IsValid()) return;

	if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(GYStateTags::State_Combat_SuperArmor);
	}
	bTagApplied = false;
}
