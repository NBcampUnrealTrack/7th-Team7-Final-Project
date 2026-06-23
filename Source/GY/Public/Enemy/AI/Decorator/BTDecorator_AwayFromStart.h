#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_AwayFromStart.generated.h"


UCLASS()
class GY_API UBTDecorator_AwayFromStart : public UBTDecorator
{
	GENERATED_BODY()
public:
	UBTDecorator_AwayFromStart();
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0"))
	float Threshold = 1500.f;
};
