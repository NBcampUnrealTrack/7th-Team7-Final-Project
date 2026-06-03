// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/GA_TransferItem.h"

#include "Core/GameplayTags/EventTags.h"
#include "Inventory/InventoryEntry.h"

UGA_TransferItem::UGA_TransferItem(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData RestTrigger;
	RestTrigger.TriggerTag = GYGameplayTags::Event_ItemContainer_Transfer;
	RestTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(RestTrigger);
}

void UGA_TransferItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	const UItemTransferPayload* Payload = Cast<UItemTransferPayload>(TriggerEventData->OptionalObject);
	if (!Payload)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TScriptInterface<IItemContainer> From = Payload->FromContainer;
	TScriptInterface<IItemContainer> To = Payload->ToContainer;

	if (!From || !To)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (From == To)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FInventoryEntry Taken;
	if (!From->TakeEntry(Payload->FromInstanceId, Taken))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!To->InsertEntry(Taken))
	{
		// 롤백
		From->InsertEntry(Taken);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
