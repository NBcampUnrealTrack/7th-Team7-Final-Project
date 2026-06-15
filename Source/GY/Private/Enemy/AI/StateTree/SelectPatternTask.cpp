#include "Enemy/AI/StateTree/SelectPatternTask.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"

bool FSelectPattern::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SelectorHandle);
	return true;
}

EStateTreeRunStatus FSelectPattern::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	if (!Selector) { return EStateTreeRunStatus::Failed; }
	if (!Data.Target){ return EStateTreeRunStatus::Failed; }

	TSubclassOf<UGameplayAbility> Selected = Selector->SelectNextPattern(Data.Target);
	if (!Selected) { return EStateTreeRunStatus::Failed; }

	return EStateTreeRunStatus::Succeeded;
}
