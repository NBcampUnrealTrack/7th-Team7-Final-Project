#include "Enemy/AI/Services/BTService_AggroUpdate.h"
#include "Enemy/GYEnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTService_AggroUpdate::UBTService_AggroUpdate()
{
	NodeName = TEXT("Aggro Update");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_AggroUpdate::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AGYEnemyAIController* EnemyAIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!EnemyAIC) return;

	APawn* ControlledPawn = EnemyAIC->GetPawn();
	if (!ControlledPawn) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	EnemyAIC->RemoveOutOfRangeActors(ControlledPawn->GetActorLocation(), EnemyAIC->GetLoseSightRadius());

	const TArray<FPerceivedActorInfo>& Perceived = EnemyAIC->GetPerceivedActors();

	if (Perceived.IsEmpty())
	{
		BB->ClearValue(EnemyBBKeys::TargetActor);
		BB->ClearValue(EnemyBBKeys::AttackPosition);

		EnemyAIC->ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	const FVector EnemyLocation = ControlledPawn->GetActorLocation();
	AActor* TopThreat = nullptr;
	float TopScore = -FLT_MAX;

	for (const FPerceivedActorInfo& Info : Perceived)
	{
		if (!Info.Actor.IsValid()) continue;

		float Score = CalculateThreatScore(EnemyLocation, Info);
		if (Score > TopScore)
		{
			TopScore = Score;
			TopThreat = Info.Actor.Get();
		}
	}

	if (TopThreat)
		BB->SetValueAsObject(EnemyBBKeys::TargetActor, TopThreat);

	//TODO 은서 : 강강술래 메타 삭제 시켜야함. 어그로 대상 지정 몇 초 동안 공격을 못하고
	//Chase 상태이면 그 Actor를 제외한 Actor를 선택하게

}

float UBTService_AggroUpdate::CalculateThreatScore(const FVector& EnemyLocation, const FPerceivedActorInfo& Info) const
{
	if (!Info.Actor.IsValid()) return -1.f;

	const float Distance = FVector::Dist(EnemyLocation, Info.Actor->GetActorLocation());
	return BaseScore - (Distance * DistanceWeight);
}
