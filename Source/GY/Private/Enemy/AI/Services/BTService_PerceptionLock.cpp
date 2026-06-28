#include "Enemy/AI/Services/BTService_PerceptionLock.h"

#include "Enemy/GYEnemyAIController.h"

UBTService_PerceptionLock::UBTService_PerceptionLock()
{
	NodeName = TEXT("Perception Lock");
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bNotifyTick = false;
}

void UBTService_PerceptionLock::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	if (auto* AI = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AI->StopPerception();
	}
}

void UBTService_PerceptionLock::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	if (auto* AI = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AI->StartPerception();
	}
}
