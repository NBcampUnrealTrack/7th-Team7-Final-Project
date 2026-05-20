// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/DoorActor.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Net/UnrealNetwork.h"
#include "WorldGimmick/DoorMovementComponent.h"


// Sets default values
ADoorActor::ADoorActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

void ADoorActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	FInteractionOption Option;
	Option.OptionTag = GYGameplayTags::Interaction_Open_Door;

	// bIsOpen == true  → 현재 열린 상태 → "닫기" 표시
	// bIsOpen == false → 현재 닫힌 상태 → "열기" 표시
	if (bIsOpen)
	{
		Option.Text = CloseText;
	}
	else
	{
		Option.Text = OpenText;
	}

	OutOptions.Add(Option);
}

void ADoorActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (HasAuthority == false)
	{
		return;
	}

	if (OptionTag != GYGameplayTags::Interaction_Open_Door)
	{
		return;
	}

	bIsOpen = !bIsOpen;
	OnRep_IsOpen();
}

void ADoorActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorActor, bIsOpen);
}

void ADoorActor::OnRep_IsOpen()
{
	for (UDoorMovementComponent* MovementComponent : Movements)
	{
		if (MovementComponent == nullptr)
		{
			continue;
		}

		if (bIsOpen)
		{
			MovementComponent->DoorOpen();
		}
		else
		{
			MovementComponent->DoorClose();
		}
	}
}
