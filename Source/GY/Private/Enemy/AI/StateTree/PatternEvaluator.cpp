#include "Enemy/AI/StateTree/PatternEvaluator.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/EnemyAggroComponent.h"
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
	Data.bHasPendingAbility = false;
}

void FPatternEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPatternSelectorComponent* Selector = Context.GetExternalDataPtr(SelectorHandle);
	UEnemyAggroComponent* Aggro = Context.GetExternalDataPtr(AggroHandle);

	if (!Selector || !Aggro)
	{
		Data.bHasReadyPattern = false;
		Data.bHasPendingAbility = false;
		return;
	}

	AActor* Target = Aggro->GetCurrentTarget();

	if (Target && Selector->GetPendingAbility() == nullptr)
	{
		Selector->SelectNextPattern(Target);
	}

	Data.bHasPendingAbility = (Selector->GetPendingAbility() != nullptr);

	if (!Target)
	{
		Data.bHasReadyPattern = false;
		return;
	}

	Data.bHasReadyPattern = Selector->HasReadyPattern(Target);
}
