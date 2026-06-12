// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Potion/GA_UseConsumable.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Inventory/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "AbilitySystemComponent.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Player/GYPlayerState.h"

UGA_UseConsumable::UGA_UseConsumable(const FObjectInitializer&)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_Item_Used;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_UseConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UInventoryComponent* InventoryComponent = nullptr;

	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AGYPlayerState* PS = Avatar->GetPlayerState<AGYPlayerState>();
	if (PS)
	{
		InventoryComponent = PS->GetInventoryComponent();
	}

	if (!InventoryComponent)
	{
		InventoryComponent = Avatar->FindComponentByClass<UInventoryComponent>();
	}

	if (!InventoryComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	TArray<FInventoryEntry> Consumables =
		InventoryComponent->GetAllEntriesByCategory(GYGameplayTags::Item_Category_Consumable);


	FGameplayTag ConsumableTag;
	if (TriggerEventData->EventTag.MatchesTagExact(GYGameplayTags::Event_Item_Used_HP))
	{
		ConsumableTag = GYGameplayTags::Consumable_ChargePool_HP;
	}
	else if (TriggerEventData->EventTag.MatchesTagExact(GYGameplayTags::Event_Item_Used_SP))
	{
		ConsumableTag = GYGameplayTags::Consumable_ChargePool_SP;
	}
	bool Succeed = false;
	for (FInventoryEntry& Entry : Consumables)
	{
		const UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		const UItemFragment_Consumable* Consumable = Def->FindFragment<UItemFragment_Consumable>();

		if (Consumable == nullptr || !Consumable->ChargePoolTag.IsValid()) continue;
		if (Consumable->ChargePoolTag != ConsumableTag) continue;
		if (!Consumable->EffectGE) continue;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(
			Consumable->EffectGE, GetAbilityLevel(), Context);
		if (!EffectSpecHandle.IsValid()) continue;

		if (InventoryComponent->TryRemoveItem(Entry.InstanceId, 1))
		{
			ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);
			Succeed = true;
			break;
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !Succeed);
}
