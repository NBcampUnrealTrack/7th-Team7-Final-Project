// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/GA_SwapSlot.h"

#include "Core/GameplayTags/EventTags.h"
#include "Inventory/InventoryEntry.h"

UGA_SwapSlot::UGA_SwapSlot(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData RestTrigger;
	RestTrigger.TriggerTag = GYGameplayTags::Event_ItemContainer_Swap;
	RestTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(RestTrigger);
}

void UGA_SwapSlot::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	const UItemSwapPayload* Payload = Cast<UItemSwapPayload>(TriggerEventData->OptionalObject);

	if (Payload)
	{
		TScriptInterface<IItemContainer> From = Payload->FromContainer;
		TScriptInterface<IItemContainer> To = Payload->ToContainer;
		FGuid FromSlot = Payload->FromSlot;
		FGuid ToSlot = Payload->ToSlot;
		if (!From || !To)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}
		if (From == To && FromSlot == ToSlot)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		const FInventoryEntry* FromEntry = From->FindEntry(FromSlot);
		if (!From->TryRemoveItem(FromSlot,FromEntry->StackCount))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		const FInventoryEntry* ToEntry = To->FindEntry(ToSlot);

		const bool bToHas = To->TryRemoveItem(FromSlot,FromEntry->StackCount);

		//롤백
		if (!To->InsertEntry(*FromEntry))
		{
			From->InsertEntry(*FromEntry);
			if (bToHas)
			{
				To->InsertEntry(*ToEntry);
			}

			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		if (bToHas)
		{
			From->InsertEntry(*ToEntry);
		}


		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
