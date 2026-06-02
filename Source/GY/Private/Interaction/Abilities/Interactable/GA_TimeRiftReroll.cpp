#include "Interaction/Abilities/Interactable/GA_TimeRiftReroll.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftReroll::UGA_TimeRiftReroll(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift_Reroll);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_TimeRift_Reroll;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_TimeRiftReroll::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	{
		UAbilityTask_WaitGameplayEvent* CloseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_TimeRift_Reroll_Exit);
		CloseTask->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
		CloseTask->ReadyForActivation();
	}
	{
		UAbilityTask_WaitGameplayEvent* ExitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_TimeRift_Exit);
		ExitTask->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
		ExitTask->ReadyForActivation();
	}
}



void UGA_TimeRiftReroll::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
