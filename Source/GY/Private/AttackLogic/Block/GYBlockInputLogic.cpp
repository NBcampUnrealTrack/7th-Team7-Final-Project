#include "AttackLogic/Block/GYBlockInputLogic.h"
#include "AttackLogic/Block/GYBlockFragment.h"
#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"

static constexpr float BlockDrainInterval = 0.1f;

void UGYBlockInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedMontageSet = nullptr;
	bEnding = false;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC) ASC->GetOwnedGameplayTags(OwnedTags);

	const UGYBlockFragment* Fragment = Ability->GetFragment<UGYBlockFragment>();
	const UGYBlockMontageFragment* MontageFragment = Ability->GetFragment<UGYBlockMontageFragment>();

	if (!Fragment)
	{
		Ability->RequestEnd(true);
		return;
	}

	const FGYBlockData* BlockData = Fragment->GetBestMatchingData(OwnedTags);

	if (!BlockData)
	{
		Ability->RequestEnd(true);
		return;
	}

	if (MontageFragment)
	{
		CachedMontageSet = MontageFragment->GetBestMatchingSet(OwnedTags);
	}

	CachedBlockAppliedTag = Fragment->BlockAppliedTag;
	CachedDrainAttribute = BlockData->StaminaDrainPerSecond.Attribute;
	CachedDrainPerSecond = BlockData->StaminaDrainPerSecond.Amount;
	CachedHitCostAttribute = BlockData->BlockHitCostAttribute;
	CachedHitCostMultiplier = BlockData->BlockHitCostMultiplier;

	if (ASC && CachedBlockAppliedTag.IsValid())
		ASC->AddLooseGameplayTag(CachedBlockAppliedTag);

	if (CachedMontageSet && CachedMontageSet->HoldMontage)
	{
		Ability->PlayMontageForLogic(CachedMontageSet->HoldMontage, 1.f);
	}

	if (CachedDrainPerSecond > 0.f && CachedDrainAttribute.IsValid())
	{
		ApplyDrainEffect();
	}
}

void UGYBlockInputLogic::ApplyDrainEffect()
{
	if (!CachedAbility.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	StaminaDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(CachedDrainAttribute)
		.AddUObject(this, &UGYBlockInputLogic::OnStaminaChanged);

	if (!CachedAbility->GetAvatarActorFromActorInfo()->HasAuthority()) return;

	const float DrainPerTick = CachedDrainPerSecond * BlockDrainInterval;

	UGameplayEffect* DrainGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_BlockStaminaDrain"));
	DrainGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	DrainGE->Period.Value = BlockDrainInterval;
	DrainGE->bExecutePeriodicEffectOnApplication = false;
	DrainGE->Modifiers.SetNum(1);
	DrainGE->Modifiers[0].ModifierMagnitude = FScalableFloat(-DrainPerTick);
	DrainGE->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
	DrainGE->Modifiers[0].Attribute = CachedDrainAttribute;

	FGameplayEffectSpec Spec(DrainGE, ASC->MakeEffectContext(), 1.f);
	DrainEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYBlockInputLogic::RemoveDrainEffect()
{
	if (!CachedAbility.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	if (DrainEffectHandle.IsValid())
		ASC->RemoveActiveGameplayEffect(DrainEffectHandle);
	if (StaminaDelegateHandle.IsValid() && CachedDrainAttribute.IsValid())
		ASC->GetGameplayAttributeValueChangeDelegate(CachedDrainAttribute).Remove(StaminaDelegateHandle);

	DrainEffectHandle = FActiveGameplayEffectHandle();
	StaminaDelegateHandle.Reset();
}

void UGYBlockInputLogic::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (bEnding) return;
	if (Data.NewValue <= 0.f)
		PlayBlockEnd();
}

void UGYBlockInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	RemoveDrainEffect();
	if (EndMontageTask) { EndMontageTask->EndTask(); EndMontageTask = nullptr; }
	if (BlockBreakTask) { BlockBreakTask->EndTask(); BlockBreakTask = nullptr; }
	RemoveBlockTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
}

TArray<FGameplayTag> UGYBlockInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Block_Hit, GYGameplayTags::Event_Block_LoopEnd };
}

void UGYBlockInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag == GYGameplayTags::Event_Block_LoopEnd)
	{
		PlayBlockEnd();
		return;
	}

	if (EventTag != GYGameplayTags::Event_Block_Hit || bEnding || !CachedAbility.IsValid()) return;
	if (!CachedHitCostAttribute.IsValid() || CachedHitCostMultiplier <= 0.f) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float Cost = Payload.EventMagnitude * CachedHitCostMultiplier;
	const float Current = ASC->GetNumericAttributeBase(CachedHitCostAttribute);
	const float NewValue = Current - Cost;
	ASC->SetNumericAttributeBase(CachedHitCostAttribute, NewValue);

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
		GYASC->NotifyAttributeChanged(CachedHitCostAttribute);

	if (NewValue <= 0.f)
		PlayBlockBreak();
}

void UGYBlockInputLogic::OnInputReleased()
{
	PlayBlockEnd();

	if (!CachedAbility.IsValid()) return;
	if (CachedAbility->IsLocallyControlled() && !CachedAbility->GetAvatarActorFromActorInfo()->HasAuthority())
	{
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(
			CachedAbility->GetAbilitySystemComponentFromActorInfo()))
		{
			GYASC->Server_SendGameplayEvent(GYGameplayTags::Event_Block_LoopEnd, FGameplayEventData());
		}
	}
}

TArray<FGameplayTag> UGYBlockInputLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Block };
}

void UGYBlockInputLogic::PlayBlockEnd()
{
	if (bEnding || !CachedAbility.IsValid()) return;
	bEnding = true;

	RemoveDrainEffect();
	RemoveBlockTag();

	if (CachedMontageSet && CachedMontageSet->EndMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->EndMontage, 1.f);
		EndMontageTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
		EndMontageTask->OnFinish.AddDynamic(this, &UGYBlockInputLogic::OnBlockEndMontageFinished);
		EndMontageTask->ReadyForActivation();
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYBlockInputLogic::PlayBlockBreak()
{
	if (bEnding || !CachedAbility.IsValid()) return;
	bEnding = true;

	RemoveDrainEffect();
	RemoveBlockTag();

	if (CachedMontageSet && CachedMontageSet->BlockBreakMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->BlockBreakMontage, 1.f);
		BlockBreakTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
		BlockBreakTask->OnFinish.AddDynamic(this, &UGYBlockInputLogic::OnBlockBreakMontageFinished);
		BlockBreakTask->ReadyForActivation();
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYBlockInputLogic::OnBlockEndMontageFinished()
{
	EndMontageTask = nullptr;
	if (CachedAbility.IsValid())
		CachedAbility->RequestEnd(false);
}

void UGYBlockInputLogic::OnBlockBreakMontageFinished()
{
	BlockBreakTask = nullptr;
	if (CachedAbility.IsValid())
		CachedAbility->RequestEnd(false);
}

void UGYBlockInputLogic::RemoveBlockTag()
{
	if (!CachedAbility.IsValid() || !CachedBlockAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	if (ASC->HasMatchingGameplayTag(CachedBlockAppliedTag))
		ASC->RemoveLooseGameplayTag(CachedBlockAppliedTag);
}
