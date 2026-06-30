#include "Interaction/Abilities/GA_TraceInteraction.h"

#include "AbilitySystemComponent.h"
#include "Character/GYCharacter.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SphereOverlap.h"
#include "Interaction/Tasks/GYAbilityTask_GrantNearbyInteraction.h"
#include "UI/GYUIMessages.h"

namespace
{
	UInteractionComponent* GetInteractionComponent(const FGameplayAbilityActorInfo* ActorInfo)
	{
		if (ActorInfo == nullptr) return nullptr;
		AGYCharacter* Character = Cast<AGYCharacter>(ActorInfo->AvatarActor.Get());
		return Character != nullptr ? Character->GetInteractionComponent() : nullptr;
	}
}

UGA_TraceInteraction::UGA_TraceInteraction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_TraceInteraction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 클라: UI 옵션 갱신용 Overlap 스캔
	if (ActorInfo->IsLocallyControlled())
	{
		UAbilityTask_WaitForInteractableTargets_SphereOverlap* WaitTask =
			UAbilityTask_WaitForInteractableTargets_SphereOverlap::WaitForInteractableTargets_SphereOverlap(
				this, InteractionScanRange, InteractionScanRate);
		WaitTask->InteractableObjectsChanged.AddDynamic(this, &UGA_TraceInteraction::OnOptionsUpdated);
		WaitTask->ReadyForActivation();
	}

	// 서버: 현재 가장 가까운 Interactable 추적
	if (ActorInfo->IsNetAuthority())
	{
		UGYAbilityTask_GrantNearbyInteraction* GrantTask =
			UGYAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(
				this, InteractionScanRange, InteractionScanRate);
		GrantTask->NearestInteractableChanged.AddDynamic(this, &UGA_TraceInteraction::OnNearestInteractableChanged);
		GrantTask->ReadyForActivation();
	}
}

void UGA_TraceInteraction::OnOptionsUpdated(const TScriptInterface<IInteractable>& Interactable)
{
	TArray<FInteractionOption> Options;
	if (Interactable)
	{
		APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
		Interactable->GatherInteractionOptions(Pawn, Options);
	}

	if (UInteractionComponent* Component = GetInteractionComponent(GetCurrentActorInfo()))
	{
		Component->SetCurrentInteractable(Interactable);
		Component->SetCurrentOptions(Options);
	}

	FGYInteractionOptionsMessage Message;
	Message.Options = Options;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_Interaction_OptionsChanged, Message);
}

void UGA_TraceInteraction::OnNearestInteractableChanged(const TScriptInterface<IInteractable>& Interactable)
{
	if (UInteractionComponent* Component = GetInteractionComponent(GetCurrentActorInfo()))
	{
		Component->SetCurrentInteractable(Interactable);
	}
}
