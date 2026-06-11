#include "Enemy/AI/StateTree/DeathTask.h"

#include "AIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

EStateTreeRunStatus FDeathTask::EnterState(FStateTreeExecutionContext& Context,
                                           const FStateTreeTransitionResult& Transition) const
{
	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	if (AI)
	{
		AI->StopMovement();

		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AI->GetPawn()))
		{
			Enemy->Die();
		}
	}

	return EStateTreeRunStatus::Running;
}
