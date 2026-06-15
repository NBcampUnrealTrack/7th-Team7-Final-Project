#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeExecutionContext.h"
#include "PhaseEvaluator.generated.h"

class UBossPhaseComponent;

USTRUCT()
struct FPhaseEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasPendingPhaseAction = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 PendingCount = 0;
};

USTRUCT(meta = (DisplayName = "Phase Evalutor"))
struct GY_API FPhaseEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPhaseEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	TStateTreeExternalDataHandle<UBossPhaseComponent,
		EStateTreeExternalDataRequirement::Optional> PhaseHandle;
};
