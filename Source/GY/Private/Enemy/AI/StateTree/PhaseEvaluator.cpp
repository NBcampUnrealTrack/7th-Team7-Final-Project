#include "Enemy/Ai/StateTree/PhaseEvaluator.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossPhaseComponent.h"

bool FPhaseEvaluator::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(PhaseHandle);
	return true;
}

void FPhaseEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bHasPendingPhaseAction = false;
	Data.PendingCount = 0;
}

void FPhaseEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossPhaseComponent* Phase = Context.GetExternalDataPtr(PhaseHandle);
	if (!Phase)
	{
		Data.bHasPendingPhaseAction = false;
		Data.PendingCount = 0;
		return;
	}

	Data.bHasPendingPhaseAction = Phase->HasPendingPhaseAction();
	Data.PendingCount = Phase->GetPendingCount();
}
