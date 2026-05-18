#include "Enemy/AI/Tasks/BTTask_MoveToNextPatrolPoint.h"
#include "Enemy/GYEnemyAIController.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToNextPatrolPoint::UBTTask_MoveToNextPatrolPoint()
{
	NodeName = TEXT("Move To Next Patrol Point");
}

EBTNodeResult::Type UBTTask_MoveToNextPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC || AIC->PatrolPoints.IsEmpty()) return EBTNodeResult::Failed;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(AIC->GetCurrentPatrolPoints());
	MoveRequest.SetAcceptanceRadius(AcceptableRadius);
	MoveRequest.SetUsePathfinding(true);

	const FPathFollowingRequestResult Result = AIC->MoveTo(MoveRequest);

	if (Result.Code == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	AIC->AdvancePatrolIndex();
	return EBTNodeResult::Succeeded;
}
