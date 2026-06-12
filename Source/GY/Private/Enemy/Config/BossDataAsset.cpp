#include "Enemy/Config/BossDataAsset.h"

const FBossPhaseSetup* UBossDataAsset::FindPhaseSetup(FName PhaseId) const
{
	if (PhaseId.IsNone()) return nullptr;
	return PhaseSetups.FindByPredicate(
		[PhaseId](const FBossPhaseSetup& S){ return S.PhaseId == PhaseId; });
}

const FBossPhaseSetup* UBossDataAsset::GetInitialPhaseSetup() const
{
	if (const FBossPhaseSetup* Found = FindPhaseSetup(InitialPhaseId))
	{
		return Found;
	}
	return PhaseSetups.Num() > 0 ? &PhaseSetups[0] : nullptr;
}
