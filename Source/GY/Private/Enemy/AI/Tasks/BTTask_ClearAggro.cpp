#include "Enemy/AI/Tasks/BTTask_ClearAggro.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Component/EnemyAggroComponent.h"

UBTTask_ClearAggro::UBTTask_ClearAggro()
{
	NodeName = TEXT("Clear Aggro");
}

EBTNodeResult::Type UBTTask_ClearAggro::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI) return EBTNodeResult::Failed;

	if (auto* Aggro = AI->FindComponentByClass<UEnemyAggroComponent>())
	{
		Aggro->ClearAllThreat();
	}
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->ClearValue(EnemyBBKeys::InvestigateLocation);
	}

	return EBTNodeResult::Succeeded;
}
