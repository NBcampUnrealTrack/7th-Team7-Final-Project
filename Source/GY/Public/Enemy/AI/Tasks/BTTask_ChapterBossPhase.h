#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChapterBossPhase.generated.h"

class UGameplayAbility;
struct FAbilityEndedData;

UCLASS()
class GY_API UBTTask_ChapterBossPhase : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_ChapterBossPhase();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	void OnAbilityEnded(const FAbilityEndedData& Data);
	void UnbindAbilityEnded();

	UPROPERTY(EditAnywhere, Category = "Phase")
	TSubclassOf<UGameplayAbility> PhaseAbilityClass;

private:
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FDelegateHandle AbilityEndedHandle;
};
