#include "Enemy/AI/StateTree/PatternEvaluator.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossAggroComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"

bool FPatternEvaluator::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SelectorHandle);
	Linker.LinkExternalData(AggroHandle);
	return true;
}

void FPatternEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bHasReadyPattern = false;
}

void FPatternEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	UBossAggroComponent* Aggro = Context.GetExternalDataPtr(AggroHandle);

	if (!Selector || !Aggro)
	{
		Data.bHasReadyPattern = false;
		return;
	}

	AActor* Target = Aggro->GetCurrentTarget();
	if (!Target)
	{
		Data.bHasReadyPattern = false;
		return;
	}

	Data.bHasReadyPattern = Selector->HasReadyPattern(Target);
}
