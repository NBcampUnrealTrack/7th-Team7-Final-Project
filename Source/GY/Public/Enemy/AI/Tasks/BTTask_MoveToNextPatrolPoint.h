#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToNextPatrolPoint.generated.h"

UCLASS()
class GY_API UBTTask_MoveToNextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_MoveToNextPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptableRadius = 50.f;
};
