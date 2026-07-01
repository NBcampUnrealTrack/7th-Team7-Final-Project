#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TriggerAbilityEvent.generated.h"

class UGameplayAbility;

UCLASS()
class GY_API UBTTask_TriggerAbilityEvent : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_TriggerAbilityEvent();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;

protected:
	UPROPERTY(EditAnywhere, Category = "GAS", meta = (Categories = "Anim"))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, Category = "BlackBoard")
	FName ClearBoolKeyOnFinish = NAME_None;

	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag ClearTagOnFinish;
private:
	void OnAbilityEnded(UGameplayAbility* Ability);
	void OnAbilityActivated(UGameplayAbility* Ability);

	UPROPERTY()
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY()
	TWeakObjectPtr<UGameplayAbility> ActiveAbility;
};
