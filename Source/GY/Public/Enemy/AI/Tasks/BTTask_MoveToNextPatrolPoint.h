#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToNextPatrolPoint.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

UCLASS()
class GY_API UBTTask_MoveToNextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_MoveToNextPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual  void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;

private:
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

private:
	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptableRadius = 50.f;
};
