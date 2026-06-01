#include "SkillTree/SkillTreeComponent.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "Net/UnrealNetwork.h"

USkillTreeComponent::USkillTreeComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USkillTreeComponent, UnlockedNodes);
}

bool USkillTreeComponent::IsNodeUnlocked(const USkillNodeDataAsset* Node) const
{
	if (!Node) return false;
	return UnlockedNodes.Contains(Node->GetPrimaryAssetId());
}

bool USkillTreeComponent::HasAllPrerequisites(const USkillNodeDataAsset* Node) const
{
	if (!Node) return false;
	for (USkillNodeDataAsset* Prereq : Node->Prerequisites)
	{
		if (!Prereq) continue;
		if (!IsNodeUnlocked(Prereq)) return false;
	}
	return true;
}

bool USkillTreeComponent::UnlockNode(const USkillNodeDataAsset* Node)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Node) return false;
	if (IsNodeUnlocked(Node)) return false;

	UnlockedNodes.Add(Node->GetPrimaryAssetId());
	OnSkillTreeChanged.Broadcast();
	return true;
}

void USkillTreeComponent::OnRep_UnlockedNodes()
{
	OnSkillTreeChanged.Broadcast();
}
