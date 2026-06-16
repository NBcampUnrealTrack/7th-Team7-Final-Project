#include "Enemy/AI/StateTree/StationaryCondition.h"

#include "StateTreeLinker.h"
#include "Enemy/GYBossCharacterBase.h"

bool FIsBossStationaryCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(BossHandle);
	return true;
}

bool FIsBossStationaryCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AGYBossCharacterBase* Boss = Context.GetExternalDataPtr(BossHandle);
	return Boss ? Boss->bIsStationary : false;
}
