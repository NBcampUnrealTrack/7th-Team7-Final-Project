// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Abilities/Interactable/GA_TimeRiftRest.h"

#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameModes/GYGameMode.h"
#include "Inventory/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"

UGA_TimeRiftRest::UGA_TimeRiftRest(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);

	FAbilityTriggerData RestTrigger;
	RestTrigger.TriggerTag = GYGameplayTags::Event_TimeRift_Rest;
	RestTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(RestTrigger);

	RestAdvanceHour = 4.f;
}

void UGA_TimeRiftRest::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActorInfo->IsNetAuthority())
	{
		if (RecoveryEffect)
		{
			UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
			if (ASC)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(RecoveryEffect, GetAbilityLevel(), Context);
				if (SpecHandle.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
				}
			}
		}
		if (UWorld* World = GetWorld())
		{
			if (AGYGameMode* GameMove = Cast<AGYGameMode>(World->GetAuthGameMode()))
			{
				GameMove->AdvanceHour(RestAdvanceHour);
			}
		}
		AActor* Avatar = GetAvatarActorFromActorInfo();

		UInventoryComponent* InventoryComponent = nullptr;
		if (APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (AGYPlayerState* GYPlayerState =  Cast<AGYPlayerState>(Pawn->GetPlayerState()))
			{
				InventoryComponent = GYPlayerState->GetInventoryComponent();
			}
		}
		if (InventoryComponent == nullptr)
		{
			InventoryComponent = Avatar->FindComponentByClass<UInventoryComponent>();
		}
		if (InventoryComponent)
		{
			for (TSoftObjectPtr<UItemDefinition> PotionDef : RefillPotionDefs)
			{
				const UItemFragment_Consumable* ConsumableFragment = PotionDef.LoadSynchronous()->FindFragment<UItemFragment_Consumable>();
				if (ConsumableFragment && ConsumableFragment->ChargePoolTag.IsValid())
				{
					const FGameplayTag PoolTag = ConsumableFragment->ChargePoolTag;

					int32 PoolSum = 0;
					for (const FInventoryEntry& Entry : InventoryComponent->GetEntries())
					{
						UItemDefinition* Def = Entry.Definition.LoadSynchronous();
						if (!Def) continue;
						const UItemFragment_Consumable* Consumable = Def->FindFragment<UItemFragment_Consumable>();
						if (Consumable && Consumable->ChargePoolTag == PoolTag)
						{
							PoolSum += Entry.StackCount;
						}
					}

					const int32 Space = FMath::Max(0, ConsumableFragment->MaxCharge - PoolSum);
					int Count = FMath::Min(10000, Space);
					FGuid OutId;
					InventoryComponent->TryAddItem(PotionDef, Count, OutId);
				}
			}
		}

	}


	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

