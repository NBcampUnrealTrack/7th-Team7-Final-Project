#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_HomeCheck.generated.h"

UCLASS()
class GY_API UBTService_HomeCheck : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_HomeCheck();
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
protected:
	UPROPERTY(EditAnywhere, Category = "Home", meta = (ClampMin = "0"))
	float Threshold = 1500.f;
};
