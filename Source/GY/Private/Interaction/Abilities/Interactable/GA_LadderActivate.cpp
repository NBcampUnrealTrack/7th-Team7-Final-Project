#include "Interaction/Abilities/Interactable/GA_LadderActivate.h"

#include "Core/GameplayTags/AbilityTags.h"
#include "WorldGimmick/Ladder.h"

UGA_LadderActivate::UGA_LadderActivate(const FObjectInitializer& ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationOwnedTags.AddTag(GYGameplayTags::Ability_Ladder_Activate);

}

void UGA_LadderActivate::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!ActorInfo->IsNetAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (ALadder* Ladder = Cast<ALadder>(GetCurrentSourceObject()))
	{
		Ladder->Activate();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
