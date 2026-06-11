#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "WaitTask.generated.h"

USTRUCT()
struct FWaitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float WaitTime = 1.f;

	float ElapsedTime = 0.f;
};

USTRUCT(meta = (DisplayName = "Wait"))
struct GY_API FWaitTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWaitInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{ return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
};
