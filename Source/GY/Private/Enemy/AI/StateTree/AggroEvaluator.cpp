#include "Enemy/AI/StateTree/AggroEvaluator.h"

#include "StateTreeLinker.h"
#include "Enemy/Component/BossAggroComponent.h"

bool FAggroEvaluator::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AggroComponentHandle);
	return true;
}

void FAggroEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.CurrentTarget = nullptr;
	Data.bHasTarget = false;
}

void FAggroEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UBossAggroComponent* Aggro = Context.GetExternalDataPtr(AggroComponentHandle);
	if (!Aggro)
	{
		Data.CurrentTarget = nullptr;
		Data.bHasTarget = false;
		return;
	}

	AActor* Target = Aggro->GetCurrentTarget();
	Data.CurrentTarget = Target;
	Data.bHasTarget = (Target != nullptr);
}
