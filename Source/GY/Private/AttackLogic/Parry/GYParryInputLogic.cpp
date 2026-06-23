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
#include "Core/GameplayTags/GameplayCueTags.h"

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

	if (const UGYParryMontageFragment* MF = Ability->GetFragment<UGYParryMontageFragment>())
	{
		CachedMontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (const UGYParryFragment* PF = Ability->GetFragment<UGYParryFragment>())
	{
		CachedParryData = PF->GetBestMatchingData(OwnedTags);
	}

	if (!CachedMontageSet || !CachedParryData)
	{
		Ability->RequestEnd(true);
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

	ParryWindowTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(CachedParryData->ParryTime, KINDA_SMALL_NUMBER));
	ParryWindowTask->OnFinish.AddDynamic(this, &UGYParryInputLogic::OnParryWindowExpired);
	ParryWindowTask->ReadyForActivation();
}

void UGYParryInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CancelPendingTasks();
	RemoveParryTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
	CachedParryData = nullptr;
}

void UGYParryInputLogic::CancelPendingTasks()
{
	if (ParryWindowTask) { ParryWindowTask->EndTask(); ParryWindowTask = nullptr; }
	if (ParryAnimTask) { ParryAnimTask->EndTask(); ParryAnimTask = nullptr; }
	if (CounterMontageTask) { CounterMontageTask->EndTask(); CounterMontageTask = nullptr; }
	if (EndMontageTask) { EndMontageTask->EndTask(); EndMontageTask = nullptr; }
}

TArray<FGameplayTag> UGYParryInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Parry_Hit };
}

void UGYParryInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	// 패리 성공
	if (EventTag != GYGameplayTags::Event_Parry_Hit || !CachedAbility.IsValid()) return;

	if (ParryWindowTask) { ParryWindowTask->EndTask(); ParryWindowTask = nullptr; }
	if (ParryAnimTask) { ParryAnimTask->EndTask(); ParryAnimTask = nullptr; }

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
	// 카메라 쉐이크
	FGameplayCueParameters Parameters;
	Parameters.Normal = FVector(1.f, 0.f, 0.f);
	ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Camera_Shake, Parameters);

	if (CachedMontageSet && CachedMontageSet->CounterMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->CounterMontage, 1.f);
		CounterMontageTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
		CounterMontageTask->OnFinish.AddDynamic(this, &UGYParryInputLogic::OnCounterMontageFinished);
		CounterMontageTask->ReadyForActivation();
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
	ParryWindowTask = nullptr;
	RemoveParryTag();

	if (!CachedAbility.IsValid() || !CachedParryData) return;

	ParryAnimTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(CachedParryData->ParryAnimTime, KINDA_SMALL_NUMBER));
	ParryAnimTask->OnFinish.AddDynamic(this, &UGYParryInputLogic::OnParryAnimExpired);
	ParryAnimTask->ReadyForActivation();
}

void UGYParryInputLogic::OnParryAnimExpired()
{
	ParryAnimTask = nullptr;
	PlayEndMontage();
}

void UGYParryInputLogic::OnCounterMontageFinished()
{
	CounterMontageTask = nullptr;
	if (CachedAbility.IsValid())
		CachedAbility->RequestEnd(false);
}

void UGYParryInputLogic::OnEndMontageFinished()
{
	EndMontageTask = nullptr;
	if (CachedAbility.IsValid())
		CachedAbility->RequestEnd(false);
}

void UGYParryInputLogic::PlayEndMontage()
{
	if (!CachedAbility.IsValid()) return;

	if (CachedMontageSet && CachedMontageSet->EndMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->EndMontage, 1.f);
		EndMontageTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
		EndMontageTask->OnFinish.AddDynamic(this, &UGYParryInputLogic::OnEndMontageFinished);
		EndMontageTask->ReadyForActivation();
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYParryInputLogic::CancelCounterMontageTask()
{
	if (CounterMontageTask)
	{
		CounterMontageTask->EndTask();
		CounterMontageTask = nullptr;
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
