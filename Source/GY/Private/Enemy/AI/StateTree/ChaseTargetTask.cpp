#include "Enemy/AI/StateTree/ChaseTargetTask.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

EStateTreeRunStatus FChaseTargetTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.f;
	Data.bMoveRequested = false;

	if (!Data.Target) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	const float Dist = FVector::Dist(
		AI->GetPawn()->GetActorLocation(),
		Data.Target->GetActorLocation());

	if (Dist <= Data.StopDistance) return EStateTreeRunStatus::Succeeded;

	FAIMoveRequest Req;
	Req.SetGoalActor(Data.Target);
	Req.SetAcceptanceRadius(Data.StopDistance);
	Req.SetUsePathfinding(true);
	Req.SetAllowPartialPath(true);

	AI->MoveTo(Req);
	Data.bMoveRequested = true;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FChaseTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;

	if (!Data.Target) return EStateTreeRunStatus::Failed;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	const float Dist = FVector::Dist(
		AI->GetPawn()->GetActorLocation(),
		Data.Target->GetActorLocation());

	if (Dist <= Data.StopDistance)
	{
		AI->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	if (Data.ElapsedTime > Data.MaxChaseTime)
	{
		AI->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FChaseTargetTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (AAIController* AI = Cast<AAIController>(Context.GetOwner()))
	{
		AI->StopMovement();
	}
}
