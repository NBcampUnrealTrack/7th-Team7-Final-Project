#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpecHandle.h"
#include "ExecutePhaseAbilityTask.generated.h"

class UBossPhaseComponent;

USTRUCT()
struct FExecutePhaseAbilityInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle ActivateHandle;

	UPROPERTY()
	TSubclassOf<UGameplayAbility> PlayingAbility;

	bool bActivated = false;
};

USTRUCT(meta = (DisplayName = "Execute Phase Ability"))
struct GY_API FExecutePhaseAbilityTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FExecutePhaseAbilityInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual  EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<UBossPhaseComponent> PhaseHandle;
};
