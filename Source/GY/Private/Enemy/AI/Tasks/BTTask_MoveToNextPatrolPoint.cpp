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
		return EBTNodeResult::Failed;

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		AIC->AdvancePatrolIndex();
		return EBTNodeResult::Succeeded;
	}

	CachedOwnerComp = &OwnerComp;
	AIC->ReceiveMoveCompleted.AddDynamic(this, &UBTTask_MoveToNextPatrolPoint::OnMoveCompleted);

	return EBTNodeResult::InProgress;
}

void UBTTask_MoveToNextPatrolPoint::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetOwner());
	if (AIC)
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this,
			&UBTTask_MoveToNextPatrolPoint::OnMoveCompleted);
	}
	CachedOwnerComp = nullptr;
}

void UBTTask_MoveToNextPatrolPoint::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (!CachedOwnerComp) return;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(
		CachedOwnerComp->GetAIOwner());

	if (AIC)
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this,
			&UBTTask_MoveToNextPatrolPoint::OnMoveCompleted);
		AIC->AdvancePatrolIndex();
	}

	const EBTNodeResult::Type NodeResult =
		(Result == EPathFollowingResult::Success)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;

	FinishLatentTask(*CachedOwnerComp, NodeResult);
	CachedOwnerComp = nullptr;
}
