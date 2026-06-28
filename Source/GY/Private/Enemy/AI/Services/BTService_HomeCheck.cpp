#include "Enemy/AI/Services/BTService_HomeCheck.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"

UBTService_HomeCheck::UBTService_HomeCheck()
{
	NodeName = TEXT("Home Check");
	Interval = 0.2f;
	RandomDeviation = 0.f;
	bCallTickOnSearchStart = true;
}

void UBTService_HomeCheck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AI = OwnerComp.GetAIOwner();
	const APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return;

	const FVector Start = BB->GetValueAsVector(EnemyBBKeys::StartLocation);
	const bool bAway = FVector::DistSquared(Pawn->GetActorLocation(), Start) > FMath::Square(Threshold);
	BB->SetValueAsBool(EnemyBBKeys::IsAwayFromHome, bAway);
}
