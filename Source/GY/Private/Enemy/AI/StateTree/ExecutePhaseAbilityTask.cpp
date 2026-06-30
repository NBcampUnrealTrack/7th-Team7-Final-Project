#include "Enemy/AI/StateTree/ExecutePhaseAbilityTask.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "StateTreeLinker.h"

#include "Enemy/Component/BossPhaseComponent.h"
#include "Logging/GYLogManager.h"

bool FExecutePhaseAbilityTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(PhaseHandle);
	return true;
}

EStateTreeRunStatus FExecutePhaseAbilityTask::EnterState(FStateTreeExecutionContext& Context,
                                                         const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ActivateHandle = FGameplayAbilitySpecHandle();
	Data.PlayingAbility = nullptr;
	Data.bActivated = false;

	UBossPhaseComponent* Phase = Context.GetExternalDataPtr(PhaseHandle);
	if (!Phase) { GY_WARN(AI, ESK, "ExecPhase: Phase 컴포넌트 없음"); return EStateTreeRunStatus::Failed; }

	TSubclassOf<UGameplayAbility> Ability = Phase->PopNextPhaseAbility();
	GY_LOG(AI, ESK, "ExecPhase: EnterState Ability=%s", *GetNameSafe(Ability.Get()));
	if (!Ability) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!ASC) return EStateTreeRunStatus::Failed;

	const bool bActivated = ASC->TryActivateAbilityByClass(Ability);
	GY_LOG(AI, ESK, "ExecPhase: TryActivateAbilityByClass(%s) → %d", *Ability->GetName(), bActivated);
	if (!bActivated) return EStateTreeRunStatus::Failed;

	Data.PlayingAbility = Ability;
	Data.bActivated = true;

	if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Ability))
	{
		Data.ActivateHandle = Spec->Handle;
	}

	Phase->NotifyPhaseStarted(Ability);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FExecutePhaseAbilityTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.bActivated) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!ASC) return EStateTreeRunStatus::Failed;

	const FGameplayAbilitySpec* Spec = nullptr;
	if (Data.ActivateHandle.IsValid())
	{
		Spec = ASC->FindAbilitySpecFromHandle(Data.ActivateHandle);
	}
	else if (Data.PlayingAbility)
	{
		Spec = ASC->FindAbilitySpecFromClass(Data.PlayingAbility);
	}

	if (!Spec || !Spec->IsActive()) return EStateTreeRunStatus::Succeeded;

	return EStateTreeRunStatus::Running;
}

void FExecutePhaseAbilityTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	const bool bWasInterrupted =
		Transition.CurrentRunStatus == EStateTreeRunStatus::Stopped ||
		Transition.CurrentRunStatus == EStateTreeRunStatus::Failed;

	GY_LOG(AI, ESK, "ExecPhase: ExitState Ability=%s Interrupted=%d Status=%d",
		*GetNameSafe(Data.PlayingAbility.Get()),
		bWasInterrupted ? 1 : 0,
		(int32)Transition.CurrentRunStatus);

	UBossPhaseComponent* Phase = Context.GetExternalDataPtr(PhaseHandle);
	if (Phase && Data.PlayingAbility)
	{
		Phase->NotifyPhaseFinished(Data.PlayingAbility);
	}

	if (bWasInterrupted && Data.ActivateHandle.IsValid())
	{
		if (AAIController* AI = Cast<AAIController>(Context.GetOwner()))
		{
			if (UAbilitySystemComponent * ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn()))
			{
				ASC->CancelAbilityHandle(Data.ActivateHandle);
			}
		}
	}
}
