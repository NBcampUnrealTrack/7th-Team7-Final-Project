#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "FaceTargetTask.generated.h"

USTRUCT()
struct FFaceTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float TurnSpeedDegPerSecond = 360.f;
};

USTRUCT(meta = (DisplayName = "Face Target"))
struct GY_API FFaceTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FFaceTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{ return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
};
