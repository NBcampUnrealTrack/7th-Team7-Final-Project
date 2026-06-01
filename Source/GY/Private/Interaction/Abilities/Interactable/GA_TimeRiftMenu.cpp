#include "Interaction/Abilities/Interactable/GA_TimeRiftMenu.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftMenu::UGA_TimeRiftMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift);
}

void UGA_TimeRiftMenu::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);


	UAbilityTask_WaitGameplayEvent* Task =  UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GYGameplayTags::Event_TimeRift_Exit);
	Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
	Task->ReadyForActivation();
}

void UGA_TimeRiftMenu::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
