// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/DoorActor.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Net/UnrealNetwork.h"
#include "Logging/GYLogManager.h"

ADoorActor::ADoorActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	DoorFrameMesh->SetupAttachment(Scene);
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<UDoorMovementComponent>(DoorComponents);
}

void ADoorActor::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOption) const
{
	FInteractionOption Option;
	Option.OptionTag = GYGameplayTags::Interaction_Open_Door;
	Option.Text = NSLOCTEXT("Door", "Open", "열기");
	OutOption.Add(Option);
}

void ADoorActor::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (HasAuthority() == false)
	{
		return;
	}

	DoorMove();
	OnRep_Open();
}

void ADoorActor::DoorMove()
{
	if (HasAuthority() == false)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("DoorMove Function Called!"));

	bIsOpen = !bIsOpen;

	OnRep_Open();
}

void ADoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorActor, bIsOpen)
}

void ADoorActor::OnRep_Open()
{
	for (UDoorMovementComponent* DoorComp : DoorComponents)
	{
		DoorComp->SetOpen(bIsOpen);
	}
}
