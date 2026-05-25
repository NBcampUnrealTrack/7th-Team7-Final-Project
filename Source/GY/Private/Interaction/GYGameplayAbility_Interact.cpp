#include "Interaction/GYGameplayAbility_Interact.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/InputTag.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/Interactable.h"
#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SphereOverlap.h"
#include "Interaction/Tasks/GYAbilityTask_GrantNearbyInteraction.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

UGYGameplayAbility_Interact::UGYGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnSpawn;
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

	if (ActorInfo->IsNetAuthority())
	{
		// 서버 입력 이벤트 대기
		UGYAbilityTask_GrantNearbyInteraction* GrantTask =
			UGYAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(
				this, InteractionScanRange, InteractionScanRate);
		GrantTask->NearestInteractableChanged.AddDynamic(this, &ThisClass::OnNearestInteractableChanged);
		GrantTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* EventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GYGameplayTags::InputTag_Interact);
		EventTask->EventReceived.AddDynamic(this, &ThisClass::OnInteractEventReceived);
		EventTask->ReadyForActivation();
	}
}

void UGYGameplayAbility_Interact::UpdateInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	//client
	TArray<FInteractionOption> Options;
	if (Interactable)
	{
		APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
		Interactable->GatherInteractionOptions(Pawn, Options);
	}

	CurrentOptions = Options;

	FGYInteractionOptionsMessage Message;
	Message.Options = Options;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_Interaction_OptionsChanged, Message);
}

void UGYGameplayAbility_Interact::OnNearestInteractableChanged(const TScriptInterface<IInteractable>& Interactable)
{
	//server
	CurrentInteractable = Interactable;
}

void UGYGameplayAbility_Interact::OnInteractEventReceived(FGameplayEventData Payload)
{
	if (!CurrentInteractable) return;

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	TArray<FInteractionOption> Options;
	CurrentInteractable->GatherInteractionOptions(Pawn, Options);
	if (Options.Num() == 0) return;

	FGameplayTag SelectedTag;
	if (Payload.InstigatorTags.Num() > 0)
	{
		SelectedTag = Payload.InstigatorTags.First();
	}
	if (!SelectedTag.IsValid()) return;

	const FInteractionOption* Selected = Options.FindByPredicate(
		[&SelectedTag](const FInteractionOption& O) { return O.OptionTag == SelectedTag; });
	if (!Selected) return;

	const FInteractionOption& Option = *Selected;

	if (Option.InteractionAbilityToGrant)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Option.InteractionAbilityToGrant))
		{
			ASC->TryActivateAbility(Spec->Handle);
		}
		return;
	}

	CurrentInteractable->OnInteract(Option.OptionTag, Pawn);
}


void UGYGameplayAbility_Interact::TriggerInteraction(FGameplayTag OptionTag)
{

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn) return;

	AGYPlayerState* PS = Cast<AGYPlayerState>(Pawn->GetPlayerState());
	if (!PS) return;

	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	if (!OptionTag.IsValid() && CurrentOptions.Num() > 0)
	{
		OptionTag = CurrentOptions[0].OptionTag;
	}
	if (!OptionTag.IsValid()) return;

	FGameplayEventData Payload;
	Payload.InstigatorTags.AddTag(OptionTag);

	ASC->Server_SendGameplayEvent(GYGameplayTags::InputTag_Interact, Payload);
}
