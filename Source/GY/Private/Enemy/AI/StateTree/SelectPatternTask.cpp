#include "Enemy/AI/StateTree/SelectPatternTask.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "Logging/GYLogManager.h"

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
	if (!Selector)
	{
		GY_WARN(AI, ESK, "SelectPattern: Selector 없음 → Failed");
		return EStateTreeRunStatus::Failed;
	}
	if (!Data.Target)
	{
		GY_WARN(AI, ESK, "SelectPattern: Target 없음 → Failed");
		return EStateTreeRunStatus::Failed;
	}

	TSubclassOf<UGameplayAbility> Selected = Selector->SelectNextPattern(Data.Target);
	GY_LOG(AI, ESK, "SelectPattern: 결과 Selected=%s Target=%s",
		Selected ? *Selected->GetName() : TEXT("NULL"),
		*GetNameSafe(Data.Target));

	if (!Selected) { return EStateTreeRunStatus::Failed; }

	return EStateTreeRunStatus::Running;
}
