// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/Abilities/Interactable/GA_SkillUnlock.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Player/GYPlayerState.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeComponent.h"

UGA_SkillUnlock::UGA_SkillUnlock(const FObjectInitializer& ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationRequiredTags.AddTag(GYStateTags::State_Interaction_TimeRift);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_SkillTree_Unlock;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_SkillUnlock::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo->IsNetAuthority() || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const USkillNodeDataAsset* Node = Cast<USkillNodeDataAsset>(TriggerEventData->OptionalObject);
	if (!Node)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	AGYPlayerState* PS = Avatar->GetPlayerState<AGYPlayerState>();
	if (!PS)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USkillTreeComponent* SkillTreeComponent = PS->GetSkillTreeComponent();
	if (!SkillTreeComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SkillTreeComponent->IsNodeUnlocked(Node))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SkillTreeComponent->HasAllPrerequisites(Node))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float CurrentSP = ASC->GetNumericAttribute(UGYPlayerAttribute::GetSkillPointAttribute());
	if (CurrentSP < 1.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (CostEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(CostEffect, GetAbilityLevel(), Context);
		if (CostSpec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data);
		}
	}

	if (Node->SkillEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle SkillSpec = ASC->MakeOutgoingSpec(Node->SkillEffect, GetAbilityLevel(), Context);
		if (SkillSpec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SkillSpec.Data);
		}
	}

	SkillTreeComponent->UnlockNode(Node);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
