#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AbilitySystemComponent.h"
#include "BTTask_ExecuteAttack.generated.h"

class UGYEnemyAttackAbilityBase;
class UGameplayAbility;

UCLASS()
class GY_API UBTTask_ExecuteAttack : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_ExecuteAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;

private:
	void OnASCAbilityEnded(const FAbilityEndedData& EndedData);

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	FGameplayAbilitySpecHandle CachedAbilityHandle;

	UPROPERTY()
	TObjectPtr<UGYEnemyAttackAbilityBase> ActiveAbility;

};
