#include "Interaction/Abilities/Interactable/GA_TimeRiftRest.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/InputTag.h"
#include "Core/GameplayTags/StateTags.h"

UGA_TimeRiftRest::UGA_TimeRiftRest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationOwnedTags.AddTag(GYStateTags::State_Interaction_TimeRift);
}

void UGA_TimeRiftRest::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	if (RecoveryEffect && ActorInfo->IsNetAuthority())
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(RecoveryEffect, GetAbilityLevel(), Context);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}

	UAbilityTask_WaitGameplayEvent* Task =  UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GYGameplayTags::InputTag_Exit);
	Task->EventReceived.AddDynamic(this, &ThisClass::OnExitEventReceived);
	Task->ReadyForActivation();
}

void UGA_TimeRiftRest::OnExitEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
