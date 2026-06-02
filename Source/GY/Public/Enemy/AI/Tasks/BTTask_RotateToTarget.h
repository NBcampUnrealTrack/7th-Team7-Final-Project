#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToTarget.generated.h"

UCLASS()
class GY_API UBTTask_RotateToTarget : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_RotateToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Rotation")
	float AcceptableAngle = 10.f;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	float RotationSpeed = 360.f;
};
