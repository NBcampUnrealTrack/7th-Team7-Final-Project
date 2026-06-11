#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpecHandle.h"
#include "PlayPatternTask.generated.h"

class AActor;
class UBossPatternSelectorComponent;

USTRUCT()
struct FPlayPatternInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle ActiveHandle;

	UPROPERTY()
	TSubclassOf<UGameplayAbility> PlayingAbility;

	bool bActivated = false;
};

USTRUCT(meta = (DisplayName = "Play Pattern"))
struct GY_API FPlayPattern : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPlayPatternInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<UBossPatternSelectorComponent> SelectorHandle;
};
