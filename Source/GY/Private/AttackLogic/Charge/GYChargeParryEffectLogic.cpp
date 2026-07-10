#include "AttackLogic/Charge/GYChargeParryEffectLogic.h"
#include "AttackLogic/Charge/GYChargeParryEffectFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"

using GYAttributeCostHelpers::ApplyReward;

void UGYChargeParryEffectLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedData = nullptr;
	bTagsApplied = false;

	const UGYChargeParryEffectFragment* Fragment = Ability->GetFragment<UGYChargeParryEffectFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	CachedData = Fragment->GetBestMatchingData(OwnedTags);
}

void UGYChargeParryEffectLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	RemoveAppliedTags();
	CachedData = nullptr;
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYChargeParryEffectLogic::GetSubscribedEventTags() const
{
	return {
		GYGameplayTags::Event_Anim_Attack_TraceBegin,
		GYGameplayTags::Event_Anim_Attack_TraceEnd,
		GYGameplayTags::Event_Parry_Hit
	};
}

void UGYChargeParryEffectLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid() || !CachedData) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	if (EventTag == GYGameplayTags::Event_Anim_Attack_TraceBegin)
	{
		if (!CachedData->AppliedTags.IsEmpty())
		{
			ASC->AddLooseGameplayTags(CachedData->AppliedTags);
			bTagsApplied = true;
		}
		return;
	}

	if (EventTag == GYGameplayTags::Event_Anim_Attack_TraceEnd)
	{
		RemoveAppliedTags();
		return;
	}

	if (EventTag == GYGameplayTags::Event_Parry_Hit)
	{
		ApplyReward(ASC, CachedData->StaminaReward);

		if (const AActor* AttackerActor = Payload.Instigator.Get())
		{
			if (UAbilitySystemComponent* AttackerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AttackerActor))
			{
				for (const FGYAttributeEffect& Effect : CachedData->ReceiverAffected)
					UGYAdditionalResourceStatics::ApplyAttributeDelta(AttackerASC, Effect.Attribute, Effect.Amount);
			}
		}
	}
}

TArray<FGameplayTag> UGYChargeParryEffectLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ChargeParryEffect };
}

void UGYChargeParryEffectLogic::RemoveAppliedTags()
{
	if (!bTagsApplied || !CachedAbility.IsValid() || !CachedData) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && !CachedData->AppliedTags.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(CachedData->AppliedTags);
	}
	bTagsApplied = false;
}
