#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponent.h"
#include "GYBossStateTreeAIComponent.generated.h"

UCLASS()
class GY_API UGYBossStateTreeAIComponent : public UStateTreeAIComponent
{
	GENERATED_BODY()

public:
	virtual bool CollectExternalData(
		const FStateTreeExecutionContext& Context,
		const UStateTree* StateTree,
		TArrayView<const FStateTreeExternalDataDesc> Descs,
		TArrayView<FStateTreeDataView> OutDataViews) const override;
};
