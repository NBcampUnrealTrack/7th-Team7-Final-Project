#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExecuteSelectedAbility.generated.h"

UCLASS()
class GY_API UBTTask_ExecuteSelectedAbility : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_ExecuteSelectedAbility();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
private:
	void OnASCAbilityEnded(const FAbilityEndedData& EndedData);

	UPROPERTY()
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	FGameplayAbilitySpecHandle CachedAbilityHandle;
};
