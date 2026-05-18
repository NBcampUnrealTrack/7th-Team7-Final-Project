#include "Enemy/AI/Tasks/BTTask_ClearBBValue.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearBBValue::UBTTask_ClearBBValue()
{
	NodeName = TEXT("Clear BB Value");
}

EBTNodeResult::Type UBTTask_ClearBBValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	BB->ClearValue(BlackboardKey.SelectedKeyName);
	return EBTNodeResult::Succeeded;
}
