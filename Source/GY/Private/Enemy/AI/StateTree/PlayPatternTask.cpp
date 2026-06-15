#include "Enemy/AI/StateTree/PlayPatternTask.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "StateTreeLinker.h"

#include "Enemy/Component/BossPatternSelectorComponent.h"

bool FPlayPattern::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SelectorHandle);
	return true;
}

EStateTreeRunStatus FPlayPattern::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ActiveHandle = FGameplayAbilitySpecHandle();
	Data.PlayingAbility = nullptr;
	Data.bActivated = false;

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	if (!Selector) return EStateTreeRunStatus::Failed;

	TSubclassOf<UGameplayAbility> Ability = Selector->ConsumePendingAbility();
	if (!Ability) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!ASC) return EStateTreeRunStatus::Failed;

	if (!ASC->TryActivateAbilityByClass(Ability)) return EStateTreeRunStatus::Failed;

	Data.PlayingAbility = Ability;
	Data.bActivated = true;

	if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Ability))
	{
		Data.ActiveHandle = Spec->Handle;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPlayPattern::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.bActivated) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!ASC) return EStateTreeRunStatus::Failed;

	const FGameplayAbilitySpec* Spec = nullptr;
	if (Data.ActiveHandle.IsValid())
	{
		Spec = ASC->FindAbilitySpecFromHandle(Data.ActiveHandle);
	}
	else if (Data.PlayingAbility)
	{
		Spec = ASC->FindAbilitySpecFromClass(Data.PlayingAbility);
	}

	if (!Spec || !Spec->IsActive()) return EStateTreeRunStatus::Succeeded;

	return EStateTreeRunStatus::Running;
}

void FPlayPattern::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	if (Selector && Data.PlayingAbility)
	{
		Selector->NotifyPatternFinished(Data.PlayingAbility);
	}

	const bool bWasInterrupted =
		Transition.CurrentRunStatus == EStateTreeRunStatus::Stopped ||
		Transition.CurrentRunStatus == EStateTreeRunStatus::Failed;

	if (bWasInterrupted && Data.ActiveHandle.IsValid())
	{
		if (AAIController* AI = Cast<AAIController>(Context.GetOwner()))
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn()))
			{
				ASC->CancelAbilityHandle(Data.ActiveHandle);
			}
		}
	}
}
