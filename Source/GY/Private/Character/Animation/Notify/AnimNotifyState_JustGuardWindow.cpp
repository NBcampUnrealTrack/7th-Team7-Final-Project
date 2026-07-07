#include "Character/Animation/Notify/AnimNotifyState_JustGuardWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"


void UAnimNotifyState_JustGuardWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
                                                   UAnimSequenceBase*, float, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		if (MeshComp->GetOwner()->HasAuthority())
		{
			ASC->AddLooseGameplayTag(GYGameplayTags::Ability_State_JustGuarding, 1,
				EGameplayTagReplicationState::TagAndCountToAll);
		}
	}
}

void UAnimNotifyState_JustGuardWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase*, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		if (MeshComp->GetOwner()->HasAuthority())
		{
			ASC->RemoveLooseGameplayTag(GYGameplayTags::Ability_State_JustGuarding, 1,
				EGameplayTagReplicationState::TagAndCountToAll);
		}
	}
}
