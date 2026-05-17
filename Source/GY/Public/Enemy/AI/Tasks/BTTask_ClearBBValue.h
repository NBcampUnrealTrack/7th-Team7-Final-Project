#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearBBValue.generated.h"

UCLASS()
class GY_API UBTTask_ClearBBValue : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_ClearBBValue();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
public:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector BlackboardKey;
};
