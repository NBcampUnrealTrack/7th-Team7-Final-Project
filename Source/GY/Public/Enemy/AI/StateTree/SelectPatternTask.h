#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "Abilities/GameplayAbility.h"
#include "SelectPatternTask.generated.h"

class AActor;
class UBossPatternSelectorComponent;

USTRUCT()
struct FSelectPatternInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "nullptr")
	TObjectPtr<AActor> Taget = nullptr;
};

USTRUCT(meta = (DisplayName = "Select Pattern"))
struct GY_API FSelectPattern : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSelectPatternInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<UBossPatternSelectorComponent> SelectorHandle;
};

