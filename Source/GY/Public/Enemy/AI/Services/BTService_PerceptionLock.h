#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PerceptionLock.generated.h"


UCLASS()
class GY_API UBTService_PerceptionLock : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_PerceptionLock();
protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
