#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

using GYAttributeCostHelpers::ApplyCost;

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	const FGYDodgeData* DodgeData = nullptr;
	const FGYDodgeMontageSet* MontageSet = nullptr;

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		DodgeData = DF->GetBestMatchingData(OwnedTags);
	}

	if (const UGYDodgeMontageFragment* MF = Ability->GetFragment<UGYDodgeMontageFragment>())
	{
		MontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (!DodgeData || !MontageSet)
	{
		Ability->RequestEnd(true);
		return;
	}

	const UGYDodgeFragment* DodgeFragment = Ability->GetFragment<UGYDodgeFragment>();
	CachedDodgeAppliedTag = DodgeFragment ? DodgeFragment->DodgeAppliedTag : FGameplayTag();

	ApplyCost(ASC, DodgeData->StaminaCost);

	const float Duration = Ability->PlayMontageForLogic(MontageSet->DodgeMontage, 1.f);

	float InvincibilityDuration = DodgeData->InvincibilityDuration;
	if (const UGYCoreStatAttributeSet* CoreStats = ASC ? ASC->GetSet<UGYCoreStatAttributeSet>() : nullptr)
	{
		InvincibilityDuration += CoreStats->GetEvasionInvincibilityTime();
	}

	if (ASC && CachedDodgeAppliedTag.IsValid() && InvincibilityDuration > 0.f)
	{
		ASC->AddLooseGameplayTag(CachedDodgeAppliedTag);

		IFrameTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(DodgeData->InvincibilityDuration, KINDA_SMALL_NUMBER));
		IFrameTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnIFrameFinished);
		IFrameTask->ReadyForActivation();
	}

	DodgeEndTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(Duration, 0.1f));
	DodgeEndTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnDodgeEndFinished);
	DodgeEndTask->ReadyForActivation();
}

void UGYDodgeInputLogic::OnIFrameFinished()
{
	IFrameTask = nullptr;
	RemoveDodgeTag();
}

void UGYDodgeInputLogic::OnDodgeEndFinished()
{
	DodgeEndTask = nullptr;
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (IFrameTask) { IFrameTask->EndTask(); IFrameTask = nullptr; }
	if (DodgeEndTask) { DodgeEndTask->EndTask(); DodgeEndTask = nullptr; }
	RemoveDodgeTag();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Dodge,
		GYGameplayTags::Ability_Fragment_DodgeMontage
	};
}

void UGYDodgeInputLogic::RemoveDodgeTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeAppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeAppliedTag);
	}
}
