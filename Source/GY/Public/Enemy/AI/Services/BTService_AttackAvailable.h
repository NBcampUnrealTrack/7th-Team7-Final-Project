#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BTService_AttackAvailable.generated.h"

UCLASS()
class GY_API UBTService_AttackAvailable : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_AttackAvailable();
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
private:
	UPROPERTY()
	TObjectPtr<UBlackboardComponent> CachedOwnerComp;
};
