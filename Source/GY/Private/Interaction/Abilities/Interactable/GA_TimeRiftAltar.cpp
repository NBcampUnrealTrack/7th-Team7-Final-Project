#include "Interaction/Abilities/Interactable/GA_TimeRiftAltar.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftAltar::UGA_TimeRiftAltar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift_Altar);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_TimeRift_Altar;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_TimeRiftAltar::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	{
		UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_TimeRift_Altar_Exit);
		Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
		Task->ReadyForActivation();
	}
	{
		UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_TimeRift_Exit);
		Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
		Task->ReadyForActivation();
	}
}

void UGA_TimeRiftAltar::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
