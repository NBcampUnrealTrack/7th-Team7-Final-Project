// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Abilities/Interactable/GA_TimeRiftSkillTree.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftSkillTree::UGA_TimeRiftSkillTree(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);

	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift_SkillTree);

	FAbilityTriggerData RestTrigger;
	RestTrigger.TriggerTag = GYGameplayTags::Event_TimeRift_SkillTree;
	RestTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(RestTrigger);
}

void UGA_TimeRiftSkillTree::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActorInfo->IsNetAuthority())
	{
		{
			UAbilityTask_WaitGameplayEvent* Task =  UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GYGameplayTags::Event_TimeRift_Exit);
			Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
			Task->ReadyForActivation();
		}
		{
			UAbilityTask_WaitGameplayEvent* Task =  UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GYGameplayTags::Event_TimeRift_SkillTree_Exit);
			Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
			Task->ReadyForActivation();
		}
	}else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_TimeRiftSkillTree::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

}
