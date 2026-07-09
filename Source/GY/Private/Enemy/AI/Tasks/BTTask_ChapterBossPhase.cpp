#include "Enemy/AI/Tasks/BTTask_ChapterBossPhase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"

UBTTask_ChapterBossPhase::UBTTask_ChapterBossPhase()
{
	bCreateNodeInstance = true;
	NodeName = TEXT("ChapterBoss Phase");
}

EBTNodeResult::Type UBTTask_ChapterBossPhase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);

	if (!ASC || !PhaseAbilityClass)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &UBTTask_ChapterBossPhase::OnAbilityEnded);

	if (!ASC->TryActivateAbilityByClass(PhaseAbilityClass))
	{
		UnbindAbilityEnded();
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::PhasePending, false);
			BB->SetValueAsObject(EnemyBBKeys::SelectedAbility,nullptr);
			BB->SetValueAsObject(EnemyBBKeys::LastUsedAbility,nullptr);
		}
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_ChapterBossPhase::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UnbindAbilityEnded();
	return EBTNodeResult::Aborted;
}

void UBTTask_ChapterBossPhase::UnbindAbilityEnded()
{
	if (!AbilityEndedHandle.IsValid()) return;

	AAIController* AIC = CachedOwnerComp.IsValid() ? CachedOwnerComp->GetAIOwner() : nullptr;
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}
	AbilityEndedHandle.Reset();
}

void UBTTask_ChapterBossPhase::OnAbilityEnded(const FAbilityEndedData& Data)
{
	if (!Data.AbilityThatEnded || Data.AbilityThatEnded->GetClass() != PhaseAbilityClass)
	{
		return;
	}

	UnbindAbilityEnded();

	if (UBehaviorTreeComponent* BTC = CachedOwnerComp.Get())
	{
		if (UBlackboardComponent* BB = BTC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::PhasePending, false);
			BB->SetValueAsObject(EnemyBBKeys::SelectedAbility, nullptr);
			BB->SetValueAsObject(EnemyBBKeys::LastUsedAbility, nullptr);
		}
		FinishLatentTask(*BTC, EBTNodeResult::Succeeded);
	}
}
