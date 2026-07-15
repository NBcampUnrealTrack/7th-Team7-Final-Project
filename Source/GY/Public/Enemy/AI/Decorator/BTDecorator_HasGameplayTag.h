#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_HasGameplayTag.generated.h"

class UAbilitySystemComponent;

UCLASS()
class GY_API UBTDecorator_HasGameplayTag : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

protected:
	UPROPERTY(EditAnywhere, Category="Tag")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category="Tag")
	bool bInvert = false;

private:
	struct FListener
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		TArray<FDelegateHandle> Handles;
	};

	mutable TMap<TWeakObjectPtr<UBehaviorTreeComponent>, FListener> Listeners;
};
