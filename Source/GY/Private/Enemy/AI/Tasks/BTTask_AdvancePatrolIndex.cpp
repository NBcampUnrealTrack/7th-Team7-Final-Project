#include "Enemy/AI/Tasks/BTTask_AdvancePatrolIndex.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"

UBTTask_AdvancePatrolIndex::UBTTask_AdvancePatrolIndex()
{
	NodeName = TEXT("Advance Patrol Index");
}

EBTNodeResult::Type UBTTask_AdvancePatrolIndex::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC || AIC->PatrolPoints.IsEmpty()) return EBTNodeResult::Failed;

	AIC->AdvancePatrolIndex();

	OwnerComp.GetBlackboardComponent()->SetValueAsVector(
		EnemyBBKeys::PatrolPosition, AIC->GetCurrentPatrolPoints());

	return EBTNodeResult::Succeeded;
}
