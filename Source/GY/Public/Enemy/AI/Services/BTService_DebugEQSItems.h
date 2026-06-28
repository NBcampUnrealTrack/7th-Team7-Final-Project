#pragma once

#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "BTService_DebugEQSItems.generated.h"

UCLASS()
class GY_API UBTService_DebugEQSItems : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_DebugEQSItems();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Debug")
	UEnvQuery* QueryTemplate;

private:
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	TWeakObjectPtr<UWorld> CachedWorld;
};
