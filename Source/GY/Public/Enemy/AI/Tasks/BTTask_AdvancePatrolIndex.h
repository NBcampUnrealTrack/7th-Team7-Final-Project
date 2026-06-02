#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AdvancePatrolIndex.generated.h"

UCLASS()
class GY_API UBTTask_AdvancePatrolIndex : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_AdvancePatrolIndex();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
