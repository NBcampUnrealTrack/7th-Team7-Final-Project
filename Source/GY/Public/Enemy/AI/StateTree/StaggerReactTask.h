#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "GameplayTagContainer.h"
#include "StaggerReactTask.generated.h"

USTRUCT()
struct FStaggerReactInstanceData
{
	GENERATED_BODY()

	/** 이벤트 Tag에 대응되는 몬타주가 MontageMap에 없을 때 사용할 폴백 Tag */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag FallbackMontageTag;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer CancelAbilitiesTags;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxDuration = 3.f;

	float ElapsedTime = 0.f;
	bool bMontagePlaying = false;
};

USTRUCT(meta = (DisplayName = "Stagger React"))
struct GY_API FStaggerReactTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStaggerReactInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{ return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

};
