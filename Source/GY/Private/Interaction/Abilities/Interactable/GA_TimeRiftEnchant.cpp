#include "Interaction/Abilities/Interactable/GA_TimeRiftEnchant.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftEnchant::UGA_TimeRiftEnchant(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift_Enchant);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_TimeRift_Enchant;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_TimeRiftEnchant::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	{
		UAbilityTask_WaitGameplayEvent* CloseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_TimeRift_Enchant_Exit);
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



void UGA_TimeRiftEnchant::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
