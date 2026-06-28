#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BTDecorator_CompareBB.generated.h"

UENUM(BlueprintType)
enum class EBBFloatCompareOp : uint8
{
	Less,
	LessEqual,
	Equal,
	GreaterEqual,
	Greater
};

UCLASS()
class GY_API UBTDecorator_CompareBB : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	EBlackboardNotificationResult OnBlackboardKeyValueChange(
		const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID);
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
protected:
	UPROPERTY(EditAnywhere, Category = "Compare")
	FBlackboardKeySelector KeyA;

	UPROPERTY(EditAnywhere, Category = "Compare")
	EBBFloatCompareOp CompareOp;

	UPROPERTY(EditAnywhere, Category = "Compare")
	FBlackboardKeySelector KeyB;

	UPROPERTY(EditAnywhere, Category = "FlowControl")
	TEnumAsByte<EBTBlackboardRestart::Type> NotifyObserver;
};
