#include "Interaction/GYGameplayAbility_Interact.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/Interactable.h"
#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SphereOverlap.h"
#include "UI/GYUIMessages.h"

UGYGameplayAbility_Interact::UGYGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGYGameplayAbility_Interact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 클라 UI용 Overlap
	if (ActorInfo->IsLocallyControlled())
	{
		UAbilityTask_WaitForInteractableTargets_SphereOverlap* WaitTask =
			UAbilityTask_WaitForInteractableTargets_SphereOverlap::WaitForInteractableTargets_SphereOverlap(
				this, InteractionScanRange, InteractionScanRate);

		WaitTask->InteractableObjectsChanged.AddDynamic(
			this, &UGYGameplayAbility_Interact::UpdateInteraction);

		WaitTask->ReadyForActivation();
	}
}

void UGYGameplayAbility_Interact::UpdateInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	CurrentInteractable = Interactable;

	TArray<FInteractionOption> Options;

	if (CurrentInteractable)
	{
		APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
		CurrentInteractable->GatherInteractionOptions(Pawn, Options);
	}

	FGYInteractionOptionsMessage Message;
	Message.Options = Options;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_Interaction_OptionsChanged, Message);
}


void UGYGameplayAbility_Interact::TriggerInteraction()
{
	if (!CurrentInteractable) return;

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	TArray<FInteractionOption> Options;
	CurrentInteractable->GatherInteractionOptions(Pawn, Options);

	//TODO 여러 상호작용 대응 해야할지 고민
	if (Options.Num() == 0) return;

	const FInteractionOption& Option = Options[0];

	if (Option.InteractionAbilityToGrant)
	{
		FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		ASC->TryActivateAbility(Handle);
	}

	CurrentInteractable->OnInteract(Option.OptionTag, Pawn);
}
