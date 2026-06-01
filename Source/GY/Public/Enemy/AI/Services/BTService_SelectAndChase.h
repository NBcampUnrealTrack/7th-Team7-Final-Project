#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTService_SelectAndChase.generated.h"

class UEnvQuery;

UCLASS()
class GY_API UBTService_SelectAndChase : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_SelectAndChase();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	void OnEQSFinished(TSharedPtr<FEnvQueryResult> Result);

private:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UEnvQuery> EQSAsset;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	bool bQueryInProgress = false;
};
