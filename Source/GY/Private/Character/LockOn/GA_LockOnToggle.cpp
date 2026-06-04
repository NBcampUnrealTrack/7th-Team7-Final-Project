// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LockOn/GA_LockOnToggle.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/Pawn.h"

UGA_LockOnToggle::UGA_LockOnToggle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_LockOnToggle::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
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

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ULockOnComponent* LockOn = Pawn->FindComponentByClass<ULockOnComponent>();
	if (!LockOn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (LockOn->IsLockedOn())
	{
		LockOn->StopLockOn();
	}
	else
	{
		LockOn->StartLockOn();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
