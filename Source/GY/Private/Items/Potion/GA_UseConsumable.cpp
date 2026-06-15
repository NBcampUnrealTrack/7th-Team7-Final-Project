#include "Items/Potion/GA_UseConsumable.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Inventory/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "AbilitySystemComponent.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Player/GYPlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Logging/GYLogManager.h"

UGA_UseConsumable::UGA_UseConsumable(const FObjectInitializer&)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 클라이언트: 몽타주만 재생
	if (!ActorInfo->IsNetAuthority())
	{
		UAnimMontage* Montage = TriggerEventData->EventTag.MatchesTagExact(GYGameplayTags::Event_Item_Used_HP)
			                        ? HPConsumableMontage.Get()
			                        : SPConsumableMontage.Get();
		if (Montage)
		{
			UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Montage, 1.f, NAME_None, true);
			Task->OnCompleted.AddDynamic(this, &UGA_UseConsumable::OnMontageCompleted);
			Task->OnBlendOut.AddDynamic(this, &UGA_UseConsumable::OnMontageCompleted);
			Task->OnCancelled.AddDynamic(this, &UGA_UseConsumable::OnMontageCancelled);
			Task->OnInterrupted.AddDynamic(this, &UGA_UseConsumable::OnMontageCancelled);
			Task->ReadyForActivation();
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
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

		if (!InventoryComponent->TryRemoveItem(Entry.InstanceId, 1)) continue;

		CachedEffectSpec = EffectSpecHandle;

		UAnimMontage* Montage = (ConsumableTag == GYGameplayTags::Consumable_ChargePool_HP)
			                        ? HPConsumableMontage.Get()
			                        : SPConsumableMontage.Get();
		if (Montage)
		{
			UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Montage, 1.f, NAME_None, true);
			Task->OnCompleted.AddDynamic(this, &UGA_UseConsumable::OnMontageCompleted);
			Task->OnBlendOut.AddDynamic(this, &UGA_UseConsumable::OnMontageCompleted);
			Task->OnCancelled.AddDynamic(this, &UGA_UseConsumable::OnMontageCancelled);
			Task->OnInterrupted.AddDynamic(this, &UGA_UseConsumable::OnMontageCancelled);
			Task->ReadyForActivation();
		}
		else
		{
			ASC->ApplyGameplayEffectSpecToSelf(*CachedEffectSpec.Data);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		return;
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

void UGA_UseConsumable::OnMontageCompleted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && CachedEffectSpec.IsValid())
		ASC->ApplyGameplayEffectSpecToSelf(*CachedEffectSpec.Data);
	GY_LOG(Content, CYS, "물약 몽타쥬");
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UseConsumable::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
