#include "Enemy/AI/Decorator/BTDecorator_AwayFromStart.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"

UBTDecorator_AwayFromStart::UBTDecorator_AwayFromStart()
{
	NodeName = TEXT("Away From StartLocation");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
}

bool UBTDecorator_AwayFromStart::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	const APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return false;

	const FVector Start = BB->GetValueAsVector(EnemyBBKeys::StartLocation);
	return FVector::DistSquared(Pawn->GetActorLocation(), Start) > FMath::Square(Threshold);

}
