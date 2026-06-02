#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "BTTask_UpdateAttackPos.generated.h"

UCLASS()
class GY_API UBTTask_UpdateAttackPos : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_UpdateAttackPos();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
private:
	void OnEQSFinished(TSharedPtr<FEnvQueryResult> Result);

	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UEnvQuery> MeleeEQSAsset;

	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UEnvQuery> RangedEQSAsset;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
};
