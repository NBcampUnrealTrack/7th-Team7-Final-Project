#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionContext.h"
#include "StationaryCondition.generated.h"

class AGYBossCharacterBase;

USTRUCT()
struct FIsBossStationaryConditionInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "Is Boss Stationary"))
struct GY_API FIsBossStationaryCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FIsBossStationaryConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AGYBossCharacterBase> BossHandle;
};
