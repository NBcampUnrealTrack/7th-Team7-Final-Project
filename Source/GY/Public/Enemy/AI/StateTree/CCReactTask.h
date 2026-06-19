#pragma once
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "CCReactTask.generated.h"

USTRUCT()
struct FCCReactInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "State.Hit"))
	FGameplayTag WaitTag;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer CancelAbilitiesTags;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float SafetyTimeout = 10.f;

	float ElapsedTime = 0.f;
};

USTRUCT(meta = (DisplayName = "CC React"))
struct GY_API FCCReactTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCCReactInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
};
