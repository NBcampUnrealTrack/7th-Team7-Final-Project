#include "Enemy/Ai/StateTree/PhaseEvaluator.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Logging/GYLogManager.h"

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
		if (Data.bHasPendingPhaseAction)
		{
			GY_WARN(AI, ESK, "PhaseEval: Phase 컴포넌트 없음");
		}
		Data.bHasPendingPhaseAction = false;
		Data.PendingCount = 0;
		return;
	}

	const bool bNew = Phase->HasPendingPhaseAction();
	const int32 NewCount = Phase->GetPendingCount();

	if (bNew != Data.bHasPendingPhaseAction || NewCount != Data.PendingCount)
	{
		GY_LOG(AI, ESK, "PhaseEval: 상태변경 HasPending[%d→%d] Count[%d→%d]",
			Data.bHasPendingPhaseAction, bNew,
			Data.PendingCount, NewCount);
	}

	Data.bHasPendingPhaseAction = bNew;
	Data.PendingCount = NewCount;
}
