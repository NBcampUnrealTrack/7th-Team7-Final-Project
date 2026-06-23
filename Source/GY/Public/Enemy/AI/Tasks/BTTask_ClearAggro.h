#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearAggro.generated.h"


UCLASS()
class GY_API UBTTask_ClearAggro : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_ClearAggro();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
