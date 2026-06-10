#include "Interaction/Abilities/GA_Interact.h"

#include "AbilitySystemComponent.h"
#include "Character/GYCharacter.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionComponent.h"

UGA_Interact::UGA_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Interact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AGYCharacter* Character = Cast<AGYCharacter>(Pawn);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (Character == nullptr || ASC == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UInteractionComponent* Component = Character->GetInteractionComponent();
	TScriptInterface<IInteractable> Target = Component != nullptr ? Component->GetCurrentInteractable() : nullptr;
	if (!Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	TArray<FInteractionOption> Options;
	Target->GatherInteractionOptions(Pawn, Options);

	const FInteractionOption* Best = nullptr;
	for (const FInteractionOption& Option : Options)
	{
		if (Best == nullptr || Option.Priority > Best->Priority)
		{
			Best = &Option;
		}
	}

	if (Best != nullptr)
	{
		if (Best->InteractionAbilityToGrant != nullptr)
		{
			if (FGameplayAbilitySpec* GrantedSpec = ASC->FindAbilitySpecFromClass(Best->InteractionAbilityToGrant))
			{
				GrantedSpec->SourceObject = Best->SourceObject;
				ASC->TryActivateAbility(GrantedSpec->Handle);
			}
		}
		else
		{
			Target->OnInteract(Best->OptionTag, Pawn);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
