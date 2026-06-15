#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeExecutionContext.h"
#include "AggroEvaluator.generated.h"

class UBossAggroComponent;

USTRUCT()
struct FAggroEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasTarget = false;

	/** 보스(Owner Pawn) 가 죽었는지 여부. StateTree 조건에서 죽음 상태 분기에 사용. */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bOwnerDead = false;
};

USTRUCT(meta = (DisplayName = "Aggro Evaluator"))
struct GY_API FAggroEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAggroEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	TStateTreeExternalDataHandle<UBossAggroComponent,
		EStateTreeExternalDataRequirement::Optional> AggroComponentHandle;
};
