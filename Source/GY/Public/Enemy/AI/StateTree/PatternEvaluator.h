#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeExecutionContext.h"
#include "PatternEvaluator.generated.h"

class UBossAggroComponent;
class UBossPatternSelectorComponent;


USTRUCT()
struct FPatternEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasReadyPattern = false;
};

USTRUCT(meta = (DisplayName = "Boss Pattern Evaluator"))
struct GY_API FPatternEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPatternEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual  void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	TStateTreeExternalDataHandle<UBossPatternSelectorComponent> SelectorHandle;
	TStateTreeExternalDataHandle<UBossAggroComponent> AggroHandle;
};
