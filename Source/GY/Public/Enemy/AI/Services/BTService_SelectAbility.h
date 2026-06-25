#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTService_SelectAbility.generated.h"

UCLASS()
class GY_API UBTService_SelectAbility : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_SelectAbility();
protected:
	void OnEQSFinished(TSharedPtr<FEnvQueryResult> Result);
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
private:
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	int32 PendingQueryID = INDEX_NONE;
};
